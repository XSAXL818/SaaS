#pragma once

#include "CoreMinimal.h"

/**
 * Axis-aligned (in the box's local space) soft-boundary math.
 * Outside distance is negative inside the box, zero on the surface, positive outside.
 * Darkness is a 0..1 scalar driven only by that distance. Teleport is a separate decision.
 */
struct FFPSAISoftBoundarySettings
{
	float FullBlackDistanceCm = 800.f;
	float HysteresisCm = 50.f;
	float PeakHoldSeconds = 0.15f;
	float BrightenSeconds = 0.75f;
	float FadeOutSeconds = 0.75f;
	float TeleportInsetCm = 350.f;
	float TeleportCooldownSeconds = 2.0f;
	float FullBlackThreshold = 0.995f;
};

struct FFPSAISoftBoundaryState
{
	float DisplayDarkness = 0.f;
	float PeakDarkness = 0.f;
	float PeakHoldRemaining = 0.f;
	bool bLatchedOutside = false;
	bool bRecovering = false;
	float RecoveryDarkness = 0.f;
	float TeleportCooldown = 0.f;
};

struct FFPSAISoftBoundaryUpdate
{
	float Darkness = 0.f;
	bool bTeleport = false;
};

struct FFPSAISoftBoundaryTeleport
{
	FVector WorldLocation = FVector::ZeroVector;
	float YawDegrees = 0.f;
	FVector InwardDirection = FVector::ForwardVector;
};

namespace FPSAISoftBoundaryMath
{
	/** BoxTransform must have unit scale. Extent is the scaled half-size in box-local axes. */
	inline float ComputeOutsideDistance(const FVector& WorldPoint, const FTransform& BoxTransform, const FVector& Extent)
	{
		const FVector Local = BoxTransform.InverseTransformPosition(WorldPoint);
		const FVector Delta = Local.GetAbs() - Extent;
		const float MaxDelta = Delta.GetMax();
		if (MaxDelta <= 0.f)
		{
			return MaxDelta;
		}

		const FVector Outside(
			FMath::Max(Delta.X, 0.f),
			FMath::Max(Delta.Y, 0.f),
			FMath::Max(Delta.Z, 0.f));
		return Outside.Size();
	}

	/** Radius of a cylinder volume: the smaller horizontal extent, per EFPSAISoftBoundaryShape::Cylinder. */
	inline float GetCylinderRadius(const FVector& Extent)
	{
		// FVector 分量是 double，FMath::Max/Min 要求两边同型，所以显式收成 float。
		return FMath::Max(0.f, static_cast<float>(FMath::Min(Extent.X, Extent.Y)));
	}

	/**
	 * Cylinder version of ComputeOutsideDistance, same sign convention: negative inside,
	 * zero on the surface, positive outside. Vertical runs on Extent.Z (half height).
	 */
	inline float ComputeCylinderOutsideDistance(
		const FVector& WorldPoint,
		const FTransform& VolumeTransform,
		float Radius,
		float HalfHeight)
	{
		const FVector Local = VolumeTransform.InverseTransformPosition(WorldPoint);
		const float HorizontalDelta = static_cast<float>(FVector2D(Local.X, Local.Y).Size()) - Radius;
		const float VerticalDelta = static_cast<float>(FMath::Abs(Local.Z)) - HalfHeight;

		if (HorizontalDelta <= 0.f && VerticalDelta <= 0.f)
		{
			// Inside: the nearer of the two surfaces, which stays negative.
			return FMath::Max(HorizontalDelta, VerticalDelta);
		}

		const float OutsideHorizontal = FMath::Max(HorizontalDelta, 0.f);
		const float OutsideVertical = FMath::Max(VerticalDelta, 0.f);
		return FMath::Sqrt(OutsideHorizontal * OutsideHorizontal + OutsideVertical * OutsideVertical);
	}

	/** Shape dispatcher. VolumeTransform must have unit scale, as with ComputeOutsideDistance. */
	inline float ComputeShapeOutsideDistance(
		const FVector& WorldPoint,
		const FTransform& VolumeTransform,
		const FVector& Extent,
		bool bCylinder)
	{
		if (bCylinder)
		{
			return ComputeCylinderOutsideDistance(WorldPoint, VolumeTransform, GetCylinderRadius(Extent), Extent.Z);
		}
		return ComputeOutsideDistance(WorldPoint, VolumeTransform, Extent);
	}

	/**
	 * How deep past the boundary the observer already is, in cm. Positive means the volume is
	 * doing its job: KeepIn is positive outside the box, KeepOut is positive inside it.
	 * Zero or negative means safe (KeepIn: inside the playable area; KeepOut: outside the pocket).
	 *
	 * This is the 3D form, kept for KeepIn and for callers that only have a scalar distance.
	 * KeepOut uses ComputeKeepOutDepth* instead: the 3D form measures the distance to the nearest
	 * face, so a box resting on the floor is dominated by its bottom face and never gets deep.
	 */
	inline float ComputeSignedDepth(float OutsideDistanceCm, bool bKeepOut)
	{
		return bKeepOut ? -OutsideDistanceCm : OutsideDistanceCm;
	}

	/**
	 * KeepOut depth in cm: how far the point has walked into the pocket *horizontally*.
	 * Vertical position is a membership gate only — a floor-resting box would otherwise be locked
	 * by the distance to its bottom face (a player standing on the ground is always close to it).
	 * Returns 0 when the point is outside the volume's vertical span, or not horizontally inside it.
	 */
	inline float ComputeKeepOutDepthBox(const FVector& WorldPoint, const FTransform& BoxTransform, const FVector& Extent)
	{
		const FVector Local = BoxTransform.InverseTransformPosition(WorldPoint);
		if (FMath::Abs(Local.Z) > Extent.Z)
		{
			return 0.f;
		}

		const float DepthX = static_cast<float>(Extent.X - FMath::Abs(Local.X));
		const float DepthY = static_cast<float>(Extent.Y - FMath::Abs(Local.Y));
		return FMath::Max(0.f, FMath::Min(DepthX, DepthY));
	}

	/** Cylinder version: same Z membership gate, depth is the radial inset toward the axis. */
	inline float ComputeKeepOutDepthCylinder(const FVector& WorldPoint, const FTransform& CylinderTransform, const FVector& Extent)
	{
		const FVector Local = CylinderTransform.InverseTransformPosition(WorldPoint);
		if (FMath::Abs(Local.Z) > Extent.Z)
		{
			return 0.f;
		}

		const float Radius = GetCylinderRadius(Extent);
		const float HorizontalDistance = static_cast<float>(FVector2D(Local.X, Local.Y).Size());
		return FMath::Max(0.f, Radius - HorizontalDistance);
	}

	/** Shape dispatcher for the horizontal KeepOut depth. VolumeTransform must have unit scale. */
	inline float ComputeKeepOutDepth(
		const FVector& WorldPoint,
		const FTransform& VolumeTransform,
		const FVector& Extent,
		bool bCylinder)
	{
		if (bCylinder)
		{
			return ComputeKeepOutDepthCylinder(WorldPoint, VolumeTransform, Extent);
		}
		return ComputeKeepOutDepthBox(WorldPoint, VolumeTransform, Extent);
	}

	inline float ComputeTargetDarkness(float OutsideDistanceCm, float FullBlackDistanceCm)
	{
		if (OutsideDistanceCm <= 0.f || FullBlackDistanceCm <= KINDA_SMALL_NUMBER)
		{
			return 0.f;
		}
		return FMath::Clamp(OutsideDistanceCm / FullBlackDistanceCm, 0.f, 1.f);
	}

	inline FFPSAISoftBoundaryUpdate UpdateState(
		FFPSAISoftBoundaryState& State,
		float OutsideDistanceCm,
		float DeltaTime,
		const FFPSAISoftBoundarySettings& Settings)
	{
		const float Dt = FMath::Max(0.f, DeltaTime);
		State.TeleportCooldown = FMath::Max(0.f, State.TeleportCooldown - Dt);

		if (!State.bLatchedOutside)
		{
			if (OutsideDistanceCm > 0.f)
			{
				State.bLatchedOutside = true;
			}
		}
		else if (OutsideDistanceCm < -Settings.HysteresisCm)
		{
			State.bLatchedOutside = false;
		}

		float EffectiveOutside = OutsideDistanceCm;
		if (State.bLatchedOutside && OutsideDistanceCm <= 0.f)
		{
			EffectiveOutside = 0.f;
		}

		const float Target = ComputeTargetDarkness(EffectiveOutside, Settings.FullBlackDistanceCm);
		const float BrightenRate = 1.f / FMath::Max(Settings.BrightenSeconds, 0.05f);

		FFPSAISoftBoundaryUpdate Result;

		if (State.bRecovering)
		{
			const float FadeRate = 1.f / FMath::Max(Settings.FadeOutSeconds, 0.05f);
			State.RecoveryDarkness = FMath::Max(0.f, State.RecoveryDarkness - FadeRate * Dt);
			State.DisplayDarkness = State.RecoveryDarkness;
			State.PeakDarkness = State.DisplayDarkness;
			State.PeakHoldRemaining = 0.f;
			if (State.RecoveryDarkness <= KINDA_SMALL_NUMBER)
			{
				State.bRecovering = false;
				State.DisplayDarkness = 0.f;
				State.PeakDarkness = 0.f;
			}
		}
		else
		{
			if (Target >= State.PeakDarkness)
			{
				State.PeakDarkness = Target;
				State.PeakHoldRemaining = Settings.PeakHoldSeconds;
			}
			else
			{
				State.PeakHoldRemaining -= Dt;
				if (State.PeakHoldRemaining <= 0.f)
				{
					State.PeakDarkness = FMath::Max(Target, State.PeakDarkness - BrightenRate * Dt);
				}
			}

			const float Driven = FMath::Max(Target, State.PeakDarkness);
			if (Driven >= State.DisplayDarkness)
			{
				State.DisplayDarkness = Driven;
			}
			else
			{
				State.DisplayDarkness = FMath::Max(Driven, State.DisplayDarkness - BrightenRate * Dt);
			}

			if (Target >= Settings.FullBlackThreshold && State.TeleportCooldown <= 0.f)
			{
				Result.bTeleport = true;
				State.bRecovering = true;
				State.RecoveryDarkness = 1.f;
				State.DisplayDarkness = 1.f;
				State.PeakDarkness = 1.f;
				State.TeleportCooldown = Settings.TeleportCooldownSeconds;
				State.bLatchedOutside = false;
			}
		}

		Result.Darkness = State.DisplayDarkness;
		return Result;
	}

	/**
	 * Horizontal unit direction from the volume's vertical axis toward the point.
	 * A point sitting exactly on the axis has no direction, so it exits through the nearest face.
	 */
	inline FVector2D ComputeHorizontalExitDirection2D(const FVector& LocalPoint, const FVector& Extent, bool bCylinder)
	{
		FVector2D Dir(LocalPoint.X, LocalPoint.Y);
		if (Dir.SizeSquared() >= KINDA_SMALL_NUMBER)
		{
			Dir.Normalize();
			return Dir;
		}

		if (bCylinder || Extent.X <= Extent.Y)
		{
			return FVector2D(1.f, 0.f);
		}
		return FVector2D(0.f, 1.f);
	}

	/** Distance from the vertical axis to the horizontal surface along UnitDir2D. */
	inline float ComputeHorizontalSurfaceDistance(const FVector2D& UnitDir2D, const FVector& Extent, bool bCylinder)
	{
		if (bCylinder)
		{
			return GetCylinderRadius(Extent);
		}

		const float Tx = Extent.X / FMath::Max(FMath::Abs(UnitDir2D.X), KINDA_SMALL_NUMBER);
		const float Ty = Extent.Y / FMath::Max(FMath::Abs(UnitDir2D.Y), KINDA_SMALL_NUMBER);
		return FMath::Min(Tx, Ty);
	}

	/**
	 * Landing SignedOffsetCm away from the horizontal surface along the center->Pawn ray:
	 * a positive offset lands outside the volume, a negative offset lands inside it.
	 * YawDegrees always faces inland; KeepOut flips it with ComputeTeleportYaw.
	 */
	inline FFPSAISoftBoundaryTeleport ComputeTeleportAlongRay(
		const FVector& PawnLocation,
		const FTransform& VolumeTransform,
		const FVector& Extent,
		bool bCylinder,
		float SignedOffsetCm)
	{
		FFPSAISoftBoundaryTeleport Result;
		const FVector Local = VolumeTransform.InverseTransformPosition(PawnLocation);
		const FVector2D Dir2D = ComputeHorizontalExitDirection2D(Local, Extent, bCylinder);
		const float SurfaceDistance = ComputeHorizontalSurfaceDistance(Dir2D, Extent, bCylinder);
		const float LandingDistance = FMath::Max(0.f, SurfaceDistance + SignedOffsetCm);

		const FVector LocalLanding(Dir2D.X * LandingDistance, Dir2D.Y * LandingDistance, 0.f);
		Result.WorldLocation = VolumeTransform.TransformPosition(LocalLanding);
		Result.WorldLocation.Z = PawnLocation.Z;

		const FVector WorldInward = VolumeTransform.TransformVector(-FVector(Dir2D.X, Dir2D.Y, 0.f)).GetSafeNormal2D();
		Result.InwardDirection = WorldInward.IsNearlyZero() ? FVector::ForwardVector : WorldInward;
		Result.YawDegrees = Result.InwardDirection.Rotation().Yaw;
		return Result;
	}

	/**
	 * Landing is InsetCm inside the nearest horizontal face along the exit direction.
	 * Yaw looks back toward the box center (into the playable area).
	 * Kept as the box-only KeepIn form on top of ComputeTeleportAlongRay.
	 */
	inline FFPSAISoftBoundaryTeleport ComputeTeleport(
		const FVector& PawnLocation,
		const FTransform& BoxTransform,
		const FVector& Extent,
		float InsetCm)
	{
		return ComputeTeleportAlongRay(PawnLocation, BoxTransform, Extent, false, -FMath::Max(0.f, InsetCm));
	}

	/**
	 * Yaw for an arbitrary landed position: faces the volume center for KeepIn
	 * (so the playable area is ahead) and away from it for KeepOut (so the pocket is behind).
	 */
	inline float ComputeFacingYaw(const FVector& LandingLocation, const FVector& VolumeCenter, bool bFaceOutward)
	{
		const FVector Outward = (LandingLocation - VolumeCenter).GetSafeNormal2D();
		if (Outward.IsNearlyZero())
		{
			return bFaceOutward ? 0.f : 180.f;
		}

		const float OutwardYaw = static_cast<float>(Outward.Rotation().Yaw);
		return bFaceOutward ? OutwardYaw : static_cast<float>(FRotator::ClampAxis(OutwardYaw + 180.0));
	}

	inline bool IsInsideBox(const FVector& WorldPoint, const FTransform& BoxTransform, const FVector& Extent, float MarginCm)
	{
		return ComputeOutsideDistance(WorldPoint, BoxTransform, Extent) <= -MarginCm;
	}
}
