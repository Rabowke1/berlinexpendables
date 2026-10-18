// Zerstörbare Tile-Welt: Grid + HP + eigene AABB-Kollision, gerendert als Instanced-Static-Mesh-Quads in Chunks.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BETypes.h"
#include "BETileWorld.generated.h"

class UBELevelAsset;
class UBETileSet;
class UInstancedStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FBEOnTileDestroyed, int32, TX, int32, TY, EBETile, Tile, FVector2D, Center);

UCLASS()
class BERLINEXPENDABLES_API ABETileWorld : public AActor
{
	GENERATED_BODY()

public:
	ABETileWorld();

	/** Wird im Editor gesetzt oder vom GameMode aus den Settings übernommen. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BE") TObjectPtr<UBELevelAsset> Level;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BE") TObjectPtr<UBETileSet> TileSet;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BE") TObjectPtr<UStaticMesh> QuadMesh;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BE") TObjectPtr<UMaterialInterface> TileMaterial;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BE", meta = (ClampMin = 4, ClampMax = 64)) int32 ChunkSize = 16;

	UPROPERTY(BlueprintAssignable, Category = "BE") FBEOnTileDestroyed OnTileDestroyed;

	UFUNCTION(BlueprintCallable, Category = "BE") void BuildFromLevel(UBELevelAsset* InLevel);

	// --- Abfragen -----------------------------------------------------------------
	UFUNCTION(BlueprintPure, Category = "BE") int32 Width() const { return W; }
	UFUNCTION(BlueprintPure, Category = "BE") int32 Height() const { return H; }
	UFUNCTION(BlueprintPure, Category = "BE") float PixelWidth() const { return W * TileSize; }
	UFUNCTION(BlueprintPure, Category = "BE") float PixelHeight() const { return H * TileSize; }
	UFUNCTION(BlueprintPure, Category = "BE") float GetTileSize() const { return TileSize; }
	UFUNCTION(BlueprintPure, Category = "BE") EBETile GetTile(int32 TX, int32 TY) const;
	UFUNCTION(BlueprintPure, Category = "BE") bool IsSolidTile(EBETile T) const { return T != EBETile::Air && T != EBETile::Platform; }
	UFUNCTION(BlueprintPure, Category = "BE") bool IsSolidAt(FVector2D P) const { return IsSolidTile(GetTile(FMath::FloorToInt(P.X / TileSize), FMath::FloorToInt(P.Y / TileSize))); }
	UFUNCTION(BlueprintPure, Category = "BE") FVector2D TileCenter(int32 TX, int32 TY) const { return FVector2D((TX + 0.5f) * TileSize, (TY + 0.5f) * TileSize); }
	UFUNCTION(BlueprintPure, Category = "BE") FVector2D SpawnPoint() const { return FVector2D(SpawnTile.X * TileSize, SpawnTile.Y * TileSize); }
	const TArray<FBEEntitySpawn>& GetSpawns() const { return Spawns; }

	// --- Zerstörung -----------------------------------------------------------------
	/** true, wenn das Tile zerstört wurde. */
	UFUNCTION(BlueprintCallable, Category = "BE") bool DamageTile(int32 TX, int32 TY, float Amount);
	/** Kreisförmiger Schaden (Explosion). Gibt Anzahl zerstörter Tiles zurück. */
	UFUNCTION(BlueprintCallable, Category = "BE") int32 DamageRadius(FVector2D Center, float Radius, float Power);

	// --- Bewegung (AABB gegen Grid) -------------------------------------------------
	bool MoveX(FBEBody& B, float DX) const;
	bool MoveY(FBEBody& B, float DY) const;
	bool OnPlatformOnly(const FBEBody& B) const;
	bool FitsAt(const FBEBody& B) const;

protected:
	virtual void BeginPlay() override;

private:
	float TileSize = 32.f;
	int32 W = 0, H = 0;
	TArray<uint8> Tiles;
	TArray<float> Hp;
	TArray<uint8> Back;
	TArray<uint8> WasSolid;
	TArray<int32> Instance;      // Instanzindex im Chunk-ISM
	TArray<FBEEntitySpawn> Spawns;
	FIntPoint SpawnTile = FIntPoint(2, 2);

	UPROPERTY() TMap<FIntPoint, TObjectPtr<UInstancedStaticMeshComponent>> Chunks;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> BackLayer;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> EarthLayer;

	void ParseRows(const TArray<FString>& Rows);
	void BuildVisuals();
	UInstancedStaticMeshComponent* MakeLayer(FName Name);
	FTransform TileTransform(int32 TX, int32 TY, float Depth) const;
	void ApplyRect(UInstancedStaticMeshComponent* ISM, int32 Idx, FName RectName, float Damage) const;
	FName VisualName(int32 TX, int32 TY) const;
	void RefreshTile(int32 TX, int32 TY);
	float MaxHp(EBETile T) const;
	int32 Index(int32 TX, int32 TY) const { return TY * W + TX; }
};
