// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "CameraLockOnTest.generated.h"

/**
 * Automation tests for the Camera Lock-On system
 */
class CAMERAPROJECT_API FCameraLockOnTest : public FAutomationTestBase
{
public:
	FCameraLockOnTest(const FString& InName, const bool bInComplexTask)
		: FAutomationTestBase(InName, bInComplexTask)
	{
	}

	virtual uint32 GetTestFlags() const override
	{
		return EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;
	}

	virtual uint32 GetRequiredDeviceNum() const override
	{
		return 1;
	}

	virtual FString GetTestSourceFileName() const override
	{
		return FString(__FILE__);
	}

	virtual int32 GetTestSourceFileLine() const override
	{
		return __LINE__;
	}

protected:
	/** Test helper to create a mock lock-on target actor */
	AActor* CreateMockLockOnTarget(UWorld* World, const FVector& Location, bool bIsValid = true);

	/** Test helper to create a character with camera lock-on component */
	class ACameraProjectCharacter* CreateTestCharacter(UWorld* World, const FVector& Location);
};

