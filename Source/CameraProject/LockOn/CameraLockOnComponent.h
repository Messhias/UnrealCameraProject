// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CameraLockOnComponent.generated.h"

class UCameraComponent;
class USpringArmComponent;
class ACharacter;

/**
 * Component that handles camera lock-on functionality similar to Dark Souls
 * Detects targets within camera field of view, selects the best target, and smoothly interpolates camera rotation
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CAMERAPROJECT_API UCameraLockOnComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCameraLockOnComponent(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	/** Enable or disable lock-on */
	UFUNCTION(BlueprintCallable, Category="LockOn")
	void SetLockOnEnabled(bool bEnabled);

	/** Toggle lock-on on/off */
	UFUNCTION(BlueprintCallable, Category="LockOn")
	void ToggleLockOn();

	/** Switch to the next target to the left */
	UFUNCTION(BlueprintCallable, Category="LockOn")
	void SwitchTargetLeft();

	/** Switch to the next target to the right */
	UFUNCTION(BlueprintCallable, Category="LockOn")
	void SwitchTargetRight();

	/** Returns true if currently locked on to a target */
	UFUNCTION(BlueprintCallable, Category="LockOn")
	bool IsLockedOn() const { return bIsLockedOn && LockedOnTarget.IsValid(); }

	/** Returns the currently locked-on target */
	UFUNCTION(BlueprintCallable, Category="LockOn")
	AActor* GetLockedOnTarget() const { return LockedOnTarget.Get(); }

protected:
	/** Find all valid targets within camera field of view */
	TArray<AActor*> FindTargetsInView() const;

	/** Check if a target is within the camera's field of view */
	static bool IsTargetInView(AActor* Target, const FVector& CameraLocation, const FVector& CameraForward, float FOV);

	/** Check if there's a clear line of sight to the target */
	bool HasLineOfSight(AActor* Target, const FVector& CameraLocation) const;

	/** Select the best target from a list of candidates */
	static AActor* SelectBestTarget(const TArray<AActor*>& Candidates, const FVector& CameraLocation, const FVector& CameraForward);

	/** Update camera rotation to face the locked-on target */
	void UpdateCameraRotation(float DeltaTime);

	/** Calculate score for a target (lower is better, targets closer to center of screen win) */
	static float CalculateTargetScore(AActor* Target, const FVector& CameraLocation, const FVector& CameraForward);

	/** Find the next target to the left or right of current target */
	AActor* FindNextTargetInDirection(bool bLeft) const;

private:
	/** Whether lock-on is currently active */
	UPROPERTY(VisibleAnywhere, Category="LockOn")
	bool bIsLockedOn = false;

	/** Currently locked-on target */
	UPROPERTY()
	TWeakObjectPtr<AActor> LockedOnTarget;

	/** Field of view angle (in degrees) used for target detection */
	UPROPERTY(EditAnywhere, Category="LockOn|Detection", meta=(ClampMin=10.0f, ClampMax=180.0f))
	float DetectionFOV = 60.0f;

	/** Maximum distance to detect targets */
	UPROPERTY(EditAnywhere, Category="LockOn|Detection", meta=(ClampMin=100.0f, ClampMax=5000.0f, Units="cm"))
	float MaxLockOnDistance = 2000.0f;

	/** Radius around character to search for targets */
	UPROPERTY(EditAnywhere, Category="LockOn|Detection", meta=(ClampMin=100.0f, ClampMax=10000.0f, Units="cm"))
	float SearchRadius = 3000.0f;

	/** Speed at which camera rotation interpolates towards target (degrees per second) */
	UPROPERTY(EditAnywhere, Category="LockOn|Camera", meta=(ClampMin=1.0f, ClampMax=720.0f, Units="deg/s"))
	float CameraRotationSpeed = 180.0f;

	/** Vertical offset from target center (in cm, positive = look higher) */
	UPROPERTY(EditAnywhere, Category="LockOn|Camera", meta=(Units="cm"))
	float VerticalOffset = 50.0f;

	/** Dead zone angle (in degrees) - camera won't rotate if target is within this angle */
	UPROPERTY(EditAnywhere, Category="LockOn|Camera", meta=(ClampMin=0.0f, ClampMax=45.0f, Units="deg"))
	float DeadZoneAngle = 2.0f;

	/** Cached reference to the owning character */
	UPROPERTY()
	ACharacter* OwnerCharacter = nullptr;

	/** Cached reference to camera component */
	UPROPERTY()
	UCameraComponent* CameraComponent = nullptr;

	/** Cached reference to spring arm component */
	UPROPERTY()
	USpringArmComponent* SpringArmComponent = nullptr;
};

