// Copyright zutemiss & dshadykh. Educational project.

#pragma once

#include "CoreMinimal.h"
#include "Weapons/FRWeaponBase.h"

#include "FRSniperRifle.generated.h"

/**
 * Bolt action rifle for the long lanes of the range.
 *
 * Its identity comes from the pause between shots and from the magnification of
 * the scope. The bolt cycle is modelled as a long fire interval, and the weapon
 * refuses to fire from the hip with any precision, so the player has to commit
 * to the scope for a headshot.
 */
UCLASS()
class FIRINGRANGE_API AFRSniperRifle : public AFRWeaponBase
{
	GENERATED_BODY()

public:
	AFRSniperRifle();

	/** True while the scope is raised. The HUD hides the crosshair when it is. */
	bool IsScoped() const;

protected:
	virtual void BuildWeaponMesh() override;

	/** Scope tube, drawn above the receiver. */
	UPROPERTY(VisibleAnywhere, Category = "Firing Range|Weapon")
	TObjectPtr<UStaticMeshComponent> ScopeMesh;

	/** Aim alpha above which the weapon counts as scoped. */
	UPROPERTY(EditDefaultsOnly, Category = "Firing Range|Optics", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ScopedAlphaThreshold = 0.85f;
};
