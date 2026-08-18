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

private:
	/** Cached copy of the mouse sensitivity setting, refreshed when settings change. */
	float CachedMouseSensitivity = 1.0f;

	/** Cached copy of the aim sensitivity multiplier. */
	float CachedAimSensitivityScale = 0.55f;

	/** Cached copy of the invert vertical axis setting. */
	bool bCachedInvertLookY = false;
};
