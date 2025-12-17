// Copyright Epic Games, Inc. All Rights Reserved.

#include "CameraLockOnComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetSystemLibrary.h"
#include "CameraProjectCharacter.h"
#include "ILockOnTarget.h"

class ILockOnTarget;

UCameraLockOnComponent::UCameraLockOnComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UCameraLockOnComponent::BeginPlay()
{
	Super::BeginPlay();

	// Cache references to owner and camera components
	OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (OwnerCharacter)
	{
		// Try to get camera from CameraProjectCharacter
		if (const ACameraProjectCharacter* CameraChar = Cast<ACameraProjectCharacter>(OwnerCharacter))
		{
			CameraComponent = CameraChar->GetFollowCamera();
			SpringArmComponent = CameraChar->GetCameraBoom();
		}
	}
}

void UCameraLockOnComponent::TickComponent(const float DeltaTime, const ELevelTick TickType,
                                           FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsLockedOn || !CameraComponent || !OwnerCharacter)
	{
		return;
	}

	// Verify current target is still valid
	if (!LockedOnTarget.IsValid())
	{
		bIsLockedOn = false;
		return;
	}

	// Check if target is still valid
	if (const ILockOnTarget* LockOnTarget = Cast<ILockOnTarget>(LockedOnTarget.Get()))
	{
		if (!LockOnTarget->IsLockOnValid())
		{
			// Target is no longer valid, try to find a new one
			const TArray<AActor*> ValidTargets = FindTargetsInView();
			if (ValidTargets.Num() > 0)
			{
				const FVector CameraLocation = CameraComponent->GetComponentLocation();
				const FVector CameraForward = CameraComponent->GetForwardVector();
				LockedOnTarget = SelectBestTarget(ValidTargets, CameraLocation, CameraForward);
				if (!LockedOnTarget.IsValid())
				{
					bIsLockedOn = false;
					return;
				}
			}
			else
			{
				bIsLockedOn = false;
				return;
			}
		}
	}

	// Update camera rotation to face target
	UpdateCameraRotation(DeltaTime);
}

void UCameraLockOnComponent::SetLockOnEnabled(const bool bEnabled)
{
	if (bEnabled == bIsLockedOn)
	{
		return;
	}

	if (bEnabled)
	{
		// Try to find and lock onto a target
		if (const TArray<AActor*> ValidTargets = FindTargetsInView(); ValidTargets.Num() > 0)
		{
			const FVector CameraLocation = CameraComponent->GetComponentLocation();
			const FVector CameraForward = CameraComponent->GetForwardVector();
			LockedOnTarget = SelectBestTarget(ValidTargets, CameraLocation, CameraForward);
			bIsLockedOn = LockedOnTarget.IsValid();
		}
		else
		{
			bIsLockedOn = false;
		}
	}
	else
	{
		bIsLockedOn = false;
		LockedOnTarget.Reset();
	}
}

void UCameraLockOnComponent::ToggleLockOn()
{
	SetLockOnEnabled(!bIsLockedOn);
}

void UCameraLockOnComponent::SwitchTargetLeft()
{
	if (!bIsLockedOn || !CameraComponent)
	{
		return;
	}

	if (AActor* NextTarget = FindNextTargetInDirection(true))
	{
		LockedOnTarget = NextTarget;
	}
}

void UCameraLockOnComponent::SwitchTargetRight()
{
	if (!bIsLockedOn || !CameraComponent)
	{
		return;
	}

	if (AActor* NextTarget = FindNextTargetInDirection(false))
	{
		LockedOnTarget = NextTarget;
	}
}

TArray<AActor*> UCameraLockOnComponent::FindTargetsInView() const
{
	TArray<AActor*> ValidTargets;

	if (!OwnerCharacter || !CameraComponent)
	{
		return ValidTargets;
	}

	const FVector CharacterLocation = OwnerCharacter->GetActorLocation();
	const FVector CameraLocation = CameraComponent->GetComponentLocation();
	const FVector CameraForward = CameraComponent->GetForwardVector();
	const float CurrentFOV = CameraComponent->FieldOfView;
	const float FOVToUse = DetectionFOV > 0.0f ? DetectionFOV : CurrentFOV;

	// Find all actors within search radius
	TArray<AActor*> OverlappingActors;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(OwnerCharacter);

	UKismetSystemLibrary::SphereOverlapActors(
		GetWorld(),
		CharacterLocation,
		SearchRadius,
		ObjectTypes,
		AActor::StaticClass(),
		ActorsToIgnore,
		OverlappingActors
	);

	// Filter to only valid lock-on targets that are in view
	for (AActor* Actor : OverlappingActors)
	{
		if (const ILockOnTarget* LockOnTarget = Cast<ILockOnTarget>(Actor))
		{
			if (!LockOnTarget->IsLockOnValid())
			{
				continue;
			}

			// Check if target is within FOV
			if (!IsTargetInView(Actor, CameraLocation, CameraForward, FOVToUse))
			{
				continue;
			}

			// Check line of sight
			if (!HasLineOfSight(Actor, CameraLocation))
			{
				continue;
			}

			// Check distance
			const FVector TargetLocation = LockOnTarget->GetLockOnLocation();
			const float Distance = FVector::Dist(CameraLocation, TargetLocation);
			if (Distance > MaxLockOnDistance)
			{
				continue;
			}

			ValidTargets.Add(Actor);
		}
	}

	return ValidTargets;
}

bool UCameraLockOnComponent::IsTargetInView(AActor* Target, const FVector& CameraLocation, const FVector& CameraForward,
                                            const float FOV)
{
	if (!Target)
	{
		return false;
	}

	const ILockOnTarget* LockOnTarget = Cast<ILockOnTarget>(Target);
	if (!LockOnTarget)
	{
		return false;
	}

	const FVector TargetLocation = LockOnTarget->GetLockOnLocation();
	const FVector DirectionToTarget = (TargetLocation - CameraLocation).GetSafeNormal();

	// Calculate angle between camera forward and direction to target
	const float DotProduct = FVector::DotProduct(CameraForward, DirectionToTarget);
	const float AngleRadians = FMath::Acos(FMath::Clamp(DotProduct, -1.0f, 1.0f));
	const float AngleDegrees = FMath::RadiansToDegrees(AngleRadians);

	// Check if angle is within FOV/2 (half FOV because it's measured from center to edge)
	const float HalfFOV = FOV / 2.0f;
	return AngleDegrees <= HalfFOV;
}

bool UCameraLockOnComponent::HasLineOfSight(AActor* Target, const FVector& CameraLocation) const
{
	if (!Target)
	{
		return false;
	}

	const ILockOnTarget* LockOnTarget = Cast<ILockOnTarget>(Target);
	if (!LockOnTarget)
	{
		return false;
	}

	const FVector TargetLocation = LockOnTarget->GetLockOnLocation();
	const FVector Direction = (TargetLocation - CameraLocation);
	const float Distance = Direction.Size();
	const FVector DirectionNormal = Direction.GetSafeNormal();

	// Perform line trace to check for obstructions
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerCharacter);
	QueryParams.AddIgnoredActor(Target);
	QueryParams.bTraceComplex = false;

	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		CameraLocation,
		CameraLocation + (DirectionNormal * Distance),
		ECC_Visibility,
		QueryParams
	);

	// If we hit something, there's no clear line of sight
	return !bHit;
}

AActor* UCameraLockOnComponent::SelectBestTarget(const TArray<AActor*>& Candidates, const FVector& CameraLocation,
                                                 const FVector& CameraForward)
{
	if (Candidates.Num() == 0)
	{
		return nullptr;
	}

	if (Candidates.Num() == 1)
	{
		return Candidates[0];
	}

	// Find the target with the best (lowest) score
	AActor* BestTarget = nullptr;
	float BestScore = MAX_FLT;

	for (AActor* Candidate : Candidates)
	{
		if (const float Score = CalculateTargetScore(Candidate, CameraLocation, CameraForward); Score < BestScore)
		{
			BestScore = Score;
			BestTarget = Candidate;
		}
	}

	return BestTarget;
}

float UCameraLockOnComponent::CalculateTargetScore(AActor* Target, const FVector& CameraLocation,
                                                   const FVector& CameraForward)
{
	if (!Target)
	{
		return MAX_FLT;
	}

	const ILockOnTarget* LockOnTarget = Cast<ILockOnTarget>(Target);
	if (!LockOnTarget)
	{
		return MAX_FLT;
	}

	const FVector TargetLocation = LockOnTarget->GetLockOnLocation();
	const FVector DirectionToTarget = (TargetLocation - CameraLocation).GetSafeNormal();

	// Calculate angle from camera forward (how far from center of screen)
	const float DotProduct = FVector::DotProduct(CameraForward, DirectionToTarget);
	const float AngleRadians = FMath::Acos(FMath::Clamp(DotProduct, -1.0f, 1.0f));
	const float AngleDegrees = FMath::RadiansToDegrees(AngleRadians);

	// Calculate distance
	const float Distance = FVector::Dist(CameraLocation, TargetLocation);

	// Score combines angle (how close to center) and distance
	// Lower angle and distance = better score
	// Weight angle more heavily so targets closer to center are preferred
	const float AngleScore = AngleDegrees * 2.0f; // Weight angle more
	const float DistanceScore = Distance / 100.0f; // Normalize distance

	// Subtract priority (higher priority = lower score)
	const int32 Priority = LockOnTarget->GetLockOnPriority();
	const float PriorityScore = -Priority * 10.0f;

	return AngleScore + DistanceScore + PriorityScore;
}

void UCameraLockOnComponent::UpdateCameraRotation(const float DeltaTime)
{
	if (!LockedOnTarget.IsValid() || !CameraComponent || !OwnerCharacter || !OwnerCharacter->GetController())
	{
		return;
	}

	const ILockOnTarget* LockOnTarget = Cast<ILockOnTarget>(LockedOnTarget.Get());
	if (!LockOnTarget)
	{
		return;
	}

	const FVector TargetLocation = LockOnTarget->GetLockOnLocation() + FVector(0.0f, 0.0f, VerticalOffset);
	const FVector CharacterLocation = OwnerCharacter->GetActorLocation();
	const FVector DesiredDirection = (TargetLocation - CharacterLocation).GetSafeNormal();

	// Calculate desired rotation
	const FRotator DesiredRotation = DesiredDirection.Rotation();

	// Get current control rotation
	const FRotator CurrentRotation = OwnerCharacter->GetControlRotation();

	// Calculate rotation delta
	FRotator RotationDelta = DesiredRotation - CurrentRotation;

	// Normalize rotation delta to the shortest path
	RotationDelta.Normalize();

	// Check if we're within dead zone
	const float YawDelta = FMath::Abs(RotationDelta.Yaw);
	if (const float PitchDelta = FMath::Abs(RotationDelta.Pitch); YawDelta <= DeadZoneAngle && PitchDelta <=
		DeadZoneAngle)
	{
		return; // Already close enough, don't rotate
	}

	// Interpolate rotation using degrees per second
	// RInterpTo uses interp speed as units per second, so we convert degrees/second to a suitable interp speed
	// Higher values = faster interpolation (typical range is 1-20, we'll use a mapping)
	const float InterpSpeed = CameraRotationSpeed / 60.0f; // Convert deg/s to a reasonable interp speed
	const FRotator NewRotation = FMath::RInterpTo(CurrentRotation, DesiredRotation, DeltaTime, InterpSpeed);

	// Apply new rotation to controller
	OwnerCharacter->GetController()->SetControlRotation(NewRotation);
}

AActor* UCameraLockOnComponent::FindNextTargetInDirection(const bool bLeft) const
{
	if (!LockedOnTarget.IsValid() || !CameraComponent || !OwnerCharacter)
	{
		return nullptr;
	}

	const FVector CameraLocation = CameraComponent->GetComponentLocation();

	const ILockOnTarget* CurrentLockOnTarget = Cast<ILockOnTarget>(LockedOnTarget.Get());
	if (!CurrentLockOnTarget)
	{
		return nullptr;
	}

	const FVector CurrentTargetLocation = CurrentLockOnTarget->GetLockOnLocation();
	const FVector DirectionToCurrentTarget = (CurrentTargetLocation - CameraLocation).GetSafeNormal();

	// Find all valid targets
	TArray<AActor*> ValidTargets = FindTargetsInView();
	if (ValidTargets.Num() <= 1)
	{
		return nullptr; // No other targets available
	}

	// Remove current target from list
	ValidTargets.Remove(LockedOnTarget.Get());

	AActor* BestTarget = nullptr;
	float BestAngle = MAX_FLT;

	for (AActor* Candidate : ValidTargets)
	{
		if (!Cast<ILockOnTarget>(Candidate))
		{
			continue;
		}

		const FVector CandidateLocation = Cast<ILockOnTarget>(Candidate)->GetLockOnLocation();
		const FVector DirectionToCandidate = (CandidateLocation - CameraLocation).GetSafeNormal();

		// Determine if candidate is in the desired direction
		if (const bool bCandidateIsLeft = FVector::DotProduct(
				FVector::CrossProduct(DirectionToCurrentTarget, DirectionToCandidate),
				CameraComponent->GetUpVector()) > 0.0f;
			bCandidateIsLeft != bLeft)
		{
			continue;
		}

		// Calculate angle between current target direction and candidate direction
		const float Angle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(
			FVector::DotProduct(DirectionToCurrentTarget, DirectionToCandidate),
			-1.0f,
			1.0f
		)));

		// Find the closest target in the desired direction
		if (Angle < BestAngle)
		{
			BestAngle = Angle;
			BestTarget = Candidate;
		}
	}

	return BestTarget;
}
