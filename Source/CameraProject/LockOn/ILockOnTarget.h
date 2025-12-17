// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ILockOnTarget.generated.h"

/**
 *  LockOnTarget interface
 *  Provides functionality for actors to be targeted by the camera lock-on system
 */
UINTERFACE(MinimalAPI, NotBlueprintable)
class ULockOnTarget : public UInterface
{
	GENERATED_BODY()
};

class ILockOnTarget
{
	GENERATED_BODY()

public:

	/** Returns the world location where the camera should lock on to this target (e.g., center of torso or head) */
	UFUNCTION(BlueprintCallable, Category="LockOn")
	virtual FVector GetLockOnLocation() const = 0;

	/** Returns true if this target is currently valid for lock-on (e.g., not dead, not destroyed) */
	UFUNCTION(BlueprintCallable, Category="LockOn")
	virtual bool IsLockOnValid() const = 0;

	/** Returns the priority of this target for lock-on selection (higher values = higher priority) */
	UFUNCTION(BlueprintCallable, Category="LockOn")
	virtual int32 GetLockOnPriority() const { return 0; }
};

