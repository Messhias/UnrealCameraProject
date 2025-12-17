// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"

/**
 * Test helper class for Camera Lock-On system tests
 * Note: This is not a test itself, just a helper class for the actual tests
 */
class FCameraLockOnTest
{
public:
	/** Test helper to create a mock lock-on target actor */
	static AActor* CreateMockLockOnTarget(UWorld* World, const FVector& Location, bool bIsValid = true);

	/** Test helper to create a character with camera lock-on component */
	static class ACameraProjectCharacter* CreateTestCharacter(UWorld* World, const FVector& Location);
};

