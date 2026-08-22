// Copyright zutemiss & dshadykh. Educational project.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

// Defines both FInputActionValue and EInputActionValueType, which appear in the
// signatures below, so the header cannot rely on a forward declaration here.
#include "InputActionValue.h"

#include "Core/FRTypes.h"

#include "FRCharacter.generated.h"

class UCameraComponent;
class UInputAction;
class UInputMappingContext;

/**
 * First person character used on the firing range.
 *
 * Input is handled with Enhanced Input. The mapping context and every input
 * action are built in C++ at runtime instead of being authored as data assets,
 * which keeps the whole project free of binary content while still using the
 * modern input pipeline rather than the deprecated axis and action mappings.
 */
UCLASS()
class FIRINGRANGE_API AFRCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AFRCharacter();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PawnClientRestart() override;

	/** Camera the player looks through. Also the origin of every aiming trace. */
	UCameraComponent* GetFirstPersonCamera() const { return FirstPersonCamera; }

	/** Scene component the weapon actor is attached to, parented to the camera. */
	USceneComponent* GetWeaponHolder() const { return WeaponHolder; }

	// -- Ammunition reserve ---------------------------------------------------
	//
	// The reserve is stored per ammunition family rather than per weapon, so a
	// box of 9 mm refills every weapon that feeds from 9 mm. The arrays are
	// indexed by EFRAmmoType.

	/** Rounds currently carried for an ammunition family. */
	int32 GetReserveAmmo(EFRAmmoType AmmoType) const;

	/** Upper limit the player may carry for an ammunition family. */
	int32 GetMaxReserveAmmo(EFRAmmoType AmmoType) const;

	/** Adds rounds up to the carry limit and returns how many were actually taken. */
	int32 AddReserveAmmo(EFRAmmoType AmmoType, int32 Amount);

	/** Removes rounds for a reload and returns how many were actually available. */
	int32 ConsumeReserveAmmo(EFRAmmoType AmmoType, int32 Amount);

	/** Restores the reserve to its starting values. Used by the pause menu restart. */
	void ResetReserveAmmo();

	/** True while the player holds the aim key. Read by the weapon and by the HUD. */
	bool IsAiming() const { return bIsAiming; }

protected:
	// -- Components -----------------------------------------------------------

	/** Eye of the character. Rotates with the control rotation. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Firing Range|Components")
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	/**
	 * Parent of the weapon view model.
	 *
	 * It is attached to the camera so the weapon follows the view exactly, and it
	 * carries the procedural offsets used by aiming, recoil and reload.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Firing Range|Components")
	TObjectPtr<USceneComponent> WeaponHolder;

	// -- Movement tuning ------------------------------------------------------

	/** Ground speed while walking. */
	UPROPERTY(EditDefaultsOnly, Category = "Firing Range|Movement", meta = (ClampMin = "100.0"))
	float WalkSpeed = 480.0f;

	/** Ground speed while the sprint key is held. */
	UPROPERTY(EditDefaultsOnly, Category = "Firing Range|Movement", meta = (ClampMin = "100.0"))
	float SprintSpeed = 760.0f;

	/** Ground speed while aiming down sights. Slower aim walk keeps shots readable. */
	UPROPERTY(EditDefaultsOnly, Category = "Firing Range|Movement", meta = (ClampMin = "50.0"))
	float AimWalkSpeed = 220.0f;

	// -- Input ----------------------------------------------------------------

	/** Runtime built mapping context, added to the local player on possession. */
	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> InputContext;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> ActionMove;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> ActionLook;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> ActionJump;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> ActionSprint;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> ActionCrouch;

	/** Creates every input action and fills the mapping context with key bindings. */
	virtual void BuildInputActions();

	/** Helper that allocates one action with the requested value type. */
	UInputAction* CreateInputAction(FName ActionName, EInputActionValueType ValueType);

	// -- Input handlers -------------------------------------------------------

	void Input_Move(const FInputActionValue& Value);
	void Input_Look(const FInputActionValue& Value);
	void Input_JumpStarted();
	void Input_JumpCompleted();
	void Input_SprintStarted();
	void Input_SprintCompleted();
	void Input_CrouchToggled();

	/** Reads the sensitivity values from the game instance into the cached fields. */
	void RefreshLookSettings();

	/** Applies the movement speed that matches the current stance. */
	virtual void UpdateMovementSpeed();

	/** True while the sprint key is held and the character may actually sprint. */
	bool bWantsToSprint = false;

	/** True while the aim key is held. */
	bool bIsAiming = false;

	// -- Ammunition configuration ---------------------------------------------

	/** Rounds carried at the start of a session, one entry per EFRAmmoType. */
	UPROPERTY(EditDefaultsOnly, Category = "Firing Range|Ammunition")
	TArray<int32> StartingReserveAmmo;

	/** Carry limit, one entry per EFRAmmoType. */
	UPROPERTY(EditDefaultsOnly, Category = "Firing Range|Ammunition")
	TArray<int32> MaxReserveAmmoPerType;

	/** Live reserve, one entry per EFRAmmoType. */
	UPROPERTY(Transient, VisibleInstanceOnly, Category = "Firing Range|Ammunition")
	TArray<int32> ReserveAmmo;

	/** Returns a valid array index for an ammunition family, or INDEX_NONE. */
	int32 GetAmmoIndex(EFRAmmoType AmmoType) const;

private:
	/** Cached copy of the mouse sensitivity setting, refreshed when settings change. */
	float CachedMouseSensitivity = 1.0f;

	/** Cached copy of the aim sensitivity multiplier. */
	float CachedAimSensitivityScale = 0.55f;

	/** Cached copy of the invert vertical axis setting. */
	bool bCachedInvertLookY = false;
};
