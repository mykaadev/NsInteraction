// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "NsInteractionViewSensor.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

/** Builds the scene of the view sensor test */
struct FNsViewSensorTestScene
{
    /** Spawns an actor with a box that blocks the visibility channel */
    static UBoxComponent* SpawnBox(UWorld* World, const FVector& Location, const FVector& Extent)
    {
        AActor* const Actor = World->SpawnActor<AActor>();
        UBoxComponent* const Box = NewObject<UBoxComponent>(Actor);
        Actor->SetRootComponent(Box);
        Box->SetBoxExtent(Extent);
        Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        Box->SetCollisionResponseToAllChannels(ECR_Ignore);
        Box->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
        Box->RegisterComponent();
        Box->SetWorldLocation(Location);
        return Box;
    }

    /** Returns the view direction at a yaw in degrees */
    static FVector Yaw(const float Degrees)
    {
        return FRotator(0.f, Degrees, 0.f).Vector();
    }
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNsInteractionViewSensorTest, "NsInteraction.ViewSensor.AimAssist",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNsInteractionViewSensorTest::RunTest(const FString& Parameters)
{
    UWorld* const World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("NsInteractionViewSensorTest"));
    if (!TestNotNull(TEXT("The test world exists"), World))
    {
        return false;
    }

    AActor* const Viewer = World->SpawnActor<AActor>();
    UNsInteractionViewSensor* const Sensor = NewObject<UNsInteractionViewSensor>(Viewer);
    Sensor->RegisterComponent();
    Sensor->TraceDistanceCm = 300.f;
    Sensor->AssistAngleDeg = 8.f;
    Sensor->AssistStickDeg = 2.f;

    // A small button 150 cm in front of the view. A 5 degree miss is 13 cm to the side, so the line trace misses it
    const UBoxComponent* const Button = FNsViewSensorTestScene::SpawnBox(World, FVector(150.f, 0.f, 0.f), FVector(5.f));
    const FVector Start = FVector::ZeroVector;
    TestTrue(TEXT("The exact aim hits the button"), Sensor->IsPartInView(Start, FNsViewSensorTestScene::Yaw(0.f), Button));
    TestTrue(TEXT("A look near the button aims at it"), Sensor->IsPartInView(Start, FNsViewSensorTestScene::Yaw(5.f), Button));
    TestFalse(TEXT("A look far from the button does not aim at it"), Sensor->IsPartInView(Start, FNsViewSensorTestScene::Yaw(20.f), Button));
    TestFalse(TEXT("A look away from the button does not aim at it"), Sensor->IsPartInView(Start, FNsViewSensorTestScene::Yaw(180.f), Button));
    TestFalse(TEXT("A look just outside the cone misses the button"), Sensor->IsPartInView(Start, FNsViewSensorTestScene::Yaw(13.f), Button));
    TestTrue(TEXT("The extra angle widens the cone"), Sensor->IsPartInView(Start, FNsViewSensorTestScene::Yaw(13.f), Button, 4.f));

    Sensor->TraceDistanceCm = 100.f;
    TestFalse(TEXT("A button out of reach does not count"), Sensor->IsPartInView(Start, FNsViewSensorTestScene::Yaw(0.f), Button));
    Sensor->TraceDistanceCm = 300.f;

    Sensor->AssistAngleDeg = 0.f;
    TestTrue(TEXT("Without the assist, the exact aim hits the button"), Sensor->IsPartInView(Start, FNsViewSensorTestScene::Yaw(0.f), Button));
    TestFalse(TEXT("Without the assist, a near look misses the button"), Sensor->IsPartInView(Start, FNsViewSensorTestScene::Yaw(5.f), Button));
    Sensor->AssistAngleDeg = 8.f;

    // A large part counts when the view is near its edge. Its center is more than 30 degrees away
    const UBoxComponent* const Board = FNsViewSensorTestScene::SpawnBox(World, FVector(150.f, 300.f, 0.f), FVector(100.f, 150.f, 2.f));
    TestTrue(TEXT("A look at the edge of a large part aims at it"), Sensor->IsPartInView(Start, FNsViewSensorTestScene::Yaw(30.f), Board));

    FNsViewSensorTestScene::SpawnBox(World, FVector(80.f, 0.f, 0.f), FVector(5.f, 100.f, 100.f));
    TestFalse(TEXT("A wall in front hides the button"), Sensor->IsPartInView(Start, FNsViewSensorTestScene::Yaw(5.f), Button));

    World->DestroyWorld(false);
    return true;
}

#endif
