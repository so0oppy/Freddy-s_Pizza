﻿#include "SB/Chica.h"
#include "SB/LocationState.h"
#include "SB/AILevel.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "HJS/FreddyPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Sound/SoundBase.h"
#include "SB/CupCake.h"
#include "Components/AudioComponent.h"

// Sets default values
AChica::AChica()
{

	PrimaryActorTick.bCanEverTick = true;

	AILevelComp = CreateDefaultSubobject<UAILevel>(TEXT("AILevelComp"));

	FootStepsAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("FootStepsAudioComponent"));
	FootStepsAudioComponent->SetupAttachment(RootComponent);
	FootStepsAudioComponent->bAutoActivate = false; // sound가 바로 재생되지 않게

	BreathAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("BreathAudioComponent"));
	BreathAudioComponent->SetupAttachment(RootComponent);
	BreathAudioComponent->bAutoActivate = false; // sound가 바로 재생되지 않게
}

// Called when the game starts or when spawned
void AChica::BeginPlay()
{
	Super::BeginPlay();
	
	UE_LOG(LogTemp, Warning, TEXT("Game Start"));

	TagArr.Add(FVector(0,0,0)); //[0]
	TagArr.Add(FindActorsWithTag(FName("Room1"))); //[1]
	TagArr.Add(FindActorsWithTag(FName("Room2"))); //[2]
	TagArr.Add(FindActorsWithTag(FName("Room3"))); //[3]
	TagArr.Add(FindActorsWithTag(FName("Room4"))); //[4]
	TagArr.Add(FindActorsWithTag(FName("Room5"))); //[5]
	TagArr.Add(FindActorsWithTag(FName("Room6"))); //[6]
	TagArr.Add(FindActorsWithTag(FName("Room7"))); //[7]
	TagArr.Add(FindActorsWithTag(FName("Room8"))); //[8]
	TagArr.Add(FindActorsWithTag(FName("Closet"))); //[9]
	TagArr.Add(FindActorsWithTag(FName("Bed"))); //[10]

	UE_LOG(LogTemp, Warning, TEXT("Room array complete"));

	CurrentState = ELocationState::IDLE;
}

void AChica::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

// Called every frame
void AChica::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	// 계속 레벨 확인 (AlLevel 에 있는 SetLevel())
	AILevelComp->SetLevel(this);
	// 계속 State 확인
	UpdateState(DeltaTime);

	// 계속 손전등On/Off, 문 Open/Close 확인
	FlashOn();
	DoorOpen();
}

void AChica::SetUpLocation(ELocationState State, float DeltaTime)
{
	if(CurrentState != State)
	{
		CurrentState = State;

		switch (State)
		{
		case ELocationState::IDLE:	Idle(DeltaTime);
			break;
		case ELocationState::MOVE:	Move();
			break;
		case ELocationState::ATTACK:	Attack();
			break;
		case ELocationState::CUPCAKE:	Cupcake();
			break;
		default:
			break;
		}
	}
}

void AChica::UpdateState(float DeltaTime)
{
	switch (CurrentState)
	{
	case ELocationState::IDLE:	Idle(DeltaTime);
		break;
	case ELocationState::MOVE:	Move();
		break;
	case ELocationState::ATTACK:	Attack();
		break;
	case ELocationState::CUPCAKE:	Cupcake();
		break;
	default:
		break;
	}
}

// Called to bind functionality to input
void AChica::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}


void AChica::Idle(float DeltaTime)
{
	// 현재 위치 == room1 || room3 || room4 || room6 || room8 가능
	if ( RoomNum == 6 )
	{
		AFreddyPlayer* FreddyPlayer = Cast<AFreddyPlayer>(UGameplayStatics::GetPlayerCharacter(GetWorld() , 0));
		AFreddyPlayer::LookAt LookState;

		if ( FreddyPlayer ) // 플레이어와 연동될 부분
		{
			LookState = FreddyPlayer->GetLookAtState();

			//	만약, 손전등 ON -> room1로 이동 (순간이동X)
			if ( bIsFlashlightOn == true && LookState == AFreddyPlayer::LookAt::Right )
			{
				StopFootStepsSound(); // 발소리 멈춤
				// 손전등 ON && Door: Close -> room8로 이동
				if ( bIsDoorClose == true )
				{
					SetActorLocation(TagArr[8]);
					RoomNum = 8;
					CurrentState = ELocationState::IDLE;
				}
				else
				{
					MoveToTaggedLocation(1);
					RoomNum = 1;
					CurrentState = ELocationState::MOVE;
				}
			}
		}
	}

	else if ( RoomNum == 8 )
	{
		AFreddyPlayer* FreddyPlayer = Cast<AFreddyPlayer>(UGameplayStatics::GetPlayerCharacter(GetWorld() , 0));
		AFreddyPlayer::LookAt LookState;

		if ( FreddyPlayer ) // 플레이어와 연동될 부분
		{
			LookState = FreddyPlayer->GetLookAtState();

			//→ 플레이어 위치 == Door && 손전등 ON : 점프스퀘어(공격) 
			if ( (LookState == AFreddyPlayer::LookAt::Right && bIsDoorClose == false) && bIsFlashlightOn == true )
			{
				CurrentState = ELocationState::ATTACK;

			}

			//→ 플레이어 위치 == Door && bCLOSE == true (일정 시간동안 CLOSE ⇒ 확률적으로 1,3,4 중 이동)
			if ( LookState == AFreddyPlayer::LookAt::Right || bIsDoorClose == true )
			{
				if ( bBSound == false )
				{
					// 숨소리 재생
					PlayBreathSound();
					bBSound = true;
				}
				else StopBreathSound();

				CurrentTime += DeltaTime;
				if ( CurrentTime > MovableTime )
				{
					TArray<int32> RoomTags = { 1, 3, 4 };
					int32 RandomIndex = FMath::RandRange(0 , RoomTags.Num() - 1);

					SetActorLocation(TagArr[RoomTags[RandomIndex]]);
					RoomNum = RoomTags[RandomIndex];

					StopBreathSound();

					CurrentTime = 0.f;

					CurrentState = ELocationState::MOVE;
				}
			}
		}
	}


	// 4.98초마다 AILevel에 있는 RandomMove() 호출 && Move로 상태전이
	CurrentTime += DeltaTime;

	if (CurrentTime > MovableTime) // 이동 가능한 시간이 되면
	{
		// RandomMove가 true일 때만 move
		if(AILevelComp->RandomMove(this, DeltaTime) == true)
		{

			if(RoomNum != 8) // room8이 아닐 때는 Move()
			{
				CurrentState = ELocationState::MOVE;
			}

			else if (RoomNum == 8)  //room8일 때 'attack, cupcake, 이동' 세가지 조건이므로 따로 분류
			{

				AFreddyPlayer* FreddyPlayer = Cast<AFreddyPlayer>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
				AFreddyPlayer::LookAt LookState;

				LookState = FreddyPlayer->GetLookAtState();

				//→ 플레이어 위치≠Door 일 때, 일정 시간 후에 컵케이크 점프스퀘어(공격) → GAME OVER
				if (LookState == AFreddyPlayer::LookAt::Main)
				{
					CurrentTime += DeltaTime;
					if (CurrentTime > MovableTime)
					{
						CurrentState = ELocationState::CUPCAKE;
						CurrentTime = 0.f;
					}
				}
			}
		}
		else
		{
			AILevelComp->RandomMove(this, DeltaTime);
		}
		CurrentTime = 0.f;
	}
}

void AChica::Move() // 손전등 켜고 있으면 1,3,4로만 이동
{
	UE_LOG(LogTemp, Warning, TEXT("Chica Move()"));
	FVector CurrentLocation = this->GetActorLocation();
	// 치카 위치가 room number 몇 인지
	//for(int32 i=1; i<TagArr.Num(); ++i)
	//{
	//	if(CurrentLocation.Equals(TagArr[i], 1.0f))
	//	{	
	//		RoomNum = i;
	//		break;
	//	}
	//}

	// room1 || room4 -> room3 가능
	if (RoomNum == 1 || RoomNum == 4)
	{
		SetActorLocation(TagArr[3]);
		RoomNum = 3;
	}
	// room3 -> room1 || room4 || room6 가능
	else if (RoomNum == 3)
	{
		TArray<int32> RoomTags = { 1, 4, 6 };
		int32 RandomIndex = FMath::RandRange(0, RoomTags.Num() - 1);

		//SetActorLocation(TagArr[RoomTags[RandomIndex]] );
		// RoomNum = RoomTags[RandomIndex];
		SetActorLocation(TagArr[6]);
		RoomNum = 6; // 나중에 위 랜덤값으로 수정
	}
	// room6 -> room3 || room8 가능
	else if (RoomNum == 6)
	{
		TArray<int32> RoomTags = { 3, 8 };
		int32 RandomIndex = FMath::RandRange(0, RoomTags.Num() - 1);

		SetActorLocation(TagArr[RoomTags[RandomIndex]]);
		// SetActorLocation(TagArr[8]);
		// RoomNum = 8;
		RoomNum = RoomTags[RandomIndex];

		if(bFSound == false )
		{
			// 발소리
			PlayFootStepsSound();
			bFSound = true;
		}
		else StopFootStepsSound();

		////	만약, 손전등 ON -> room1로 이동 (순간이동X)
		//if(bIsFlashlightOn == true) 
		//{
		//	StopFootStepsSound(); // 발소리 멈춤
		//	// 손전등 ON && Door: Close -> room8로 이동
		//	if (bIsDoorClose == true)
		//	{
		//		SetActorLocation(TagArr[8]);
		//		RoomNum = 8;
		//		CurrentState = ELocationState::IDLE;
		//	}
		//	else
		//	{
		//		MoveToTaggedLocation(1);
		//		RoomNum = 1;

		//	}
		//}
	}

	CurrentState = ELocationState::IDLE;
}

void AChica::Attack()
{
	// 점프스퀘어 anim 재생

	// 테스트용 -> 카메라 앞으로 SetActorLocation
	AFreddyPlayer* FreddyPlayer = Cast<AFreddyPlayer>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	FTransform JmpScare = FreddyPlayer->GetCameraTransform();
	JmpScare.SetLocation(JmpScare.GetLocation() - FVector(0 , 100 , 60)); // 위치 조정
	FRotator rot = JmpScare.GetRotation().Rotator();
	rot.Yaw += 90.0;
	JmpScare.SetRotation(rot.Quaternion());
	SetActorTransform(JmpScare); // 카메라 위치로 이동 (점프스케어)

	if ( bJSound == false )
	{
		// 점프스케어 소리 재생
		UGameplayStatics::PlaySound2D(this , JumpScareSFX);
		bJSound = true;

		FreddyPlayer->OnDie();

		UE_LOG(LogTemp , Warning , TEXT("Chica Attack !"));
	}
	// 게임 오버
	
}

void AChica::Cupcake()
{
	// 컵케이크 점프스퀘어 anim 재생

	// 테스트용 -> 카메라 앞으로 SetActorLocation
	AFreddyPlayer* FreddyPlayer = Cast<AFreddyPlayer>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	AActor* CupCakeInstance = UGameplayStatics::GetActorOfClass(GetWorld(), ACupCake::StaticClass());
	ACupCake* CupCake = Cast<ACupCake>(CupCakeInstance);
	FTransform JmpScare = FreddyPlayer->GetCameraTransform();
	JmpScare.SetLocation(JmpScare.GetLocation() - FVector(0 , 300 , 0)); // 위치 조정


	if(CupCake)
	{
		// 컵케이크 임시 큐브로
		CupCake->SetActorTransform(JmpScare); // 카메라 위치로 이동 (점프스케어)

		if(bJSound == false)
		{
			// 점프스케어 소리 재생
			UGameplayStatics::PlaySound2D(this , JumpScareSFX);
			bJSound = true;

			FreddyPlayer->OnDie();

			UE_LOG(LogTemp , Warning , TEXT("CupCake Attack !"));
		}
	}

	// 게임 오버
}

FVector AChica::FindActorsWithTag(FName Tag)
{
	// 각 방의 위치 정보를 태그로 받아와서 배열로 저장
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), Tag, FoundActors);

	if (FoundActors.Num() > 0)
	{
		AActor* TargetActor = FoundActors[0];
		FVector TargetLocation = TargetActor->GetActorLocation();

		return TargetLocation;
		// 배열 인덱스 값 = 방 번호
	}

	return FVector::ZeroVector; // FoundActors가 비어있을 경우, 기본값 반환
}

void AChica::MoveToTaggedLocation(int32 room)
{
	GetController()->StopMovement();

	ACharacter* Character = Cast<ACharacter>(this);
	if (Character)
	{ 
		Character->bUseControllerRotationYaw = false; // 캐릭터 회전을 잠금
		Character->GetCharacterMovement()->bOrientRotationToMovement = false; // 이동 방향으로 회전하지 않음
	}

	AAIController* AIController = Cast<AAIController>(GetController());
	if (AIController)
	{
		FAIMoveRequest MoveRequest;
		MoveRequest.SetGoalLocation(TagArr[room]);
		MoveRequest.SetAcceptanceRadius(5.0f); // 목표 위치에 도달하는 범위 설정

		FNavPathSharedPtr NavPath;
		EPathFollowingRequestResult::Type MoveResult = AIController->MoveTo(MoveRequest, &NavPath);

		// 이동 요청 결과 로그 출력
		switch (MoveResult)
		{
		case EPathFollowingRequestResult::Failed:
			UE_LOG(LogTemp, Warning, TEXT("MoveTo request failed."));
			break;
		case EPathFollowingRequestResult::AlreadyAtGoal:
			UE_LOG(LogTemp, Warning, TEXT("Already at goal location."));
			break;
		case EPathFollowingRequestResult::RequestSuccessful:
			UE_LOG(LogTemp, Warning, TEXT("MoveTo request successful."));
			break;
		}
	}

	if (room == 1)
	{
		RoomNum = 1;
		GetWorld()->GetTimerManager().SetTimer(Handle, this, &AChica::CanMove, MovableTime, false);
	}
}

void AChica::CanMove()
{
	RoomNum = 1;

	SetActorLocation(TagArr[1]);
	CurrentState = ELocationState::IDLE;
}

// 손전등 상태 가져오는 함수
void AChica::FlashOn()
{
	AFreddyPlayer* FreddyPlayer = Cast<AFreddyPlayer>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	if(FreddyPlayer)
	{
		bIsFlashlightOn = FreddyPlayer->GetFlash();
	}
}

// 문 상태 가져오는 함수
void AChica::DoorOpen()
{
	AFreddyPlayer* FreddyPlayer = Cast<AFreddyPlayer>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));

	if(FreddyPlayer)
	{
		bIsDoorClose = FreddyPlayer->GetrCloseDoor();
	}
}

void AChica::PlayFootStepsSound()
{
	if ( FootStepsSFX && !FootStepsAudioComponent->IsPlaying() )
	{
		FootStepsAudioComponent->SetSound(FootStepsSFX);
		FootStepsAudioComponent->Play();
	}
}

void AChica::StopFootStepsSound()
{
	if ( FootStepsAudioComponent->IsPlaying() )
	{
		FootStepsAudioComponent->Stop();
	}
}

void AChica::PlayBreathSound()
{
	if ( BreathSFX && !BreathAudioComponent->IsPlaying() )
	{
		BreathAudioComponent->SetSound(BreathSFX);
		BreathAudioComponent->Play();
	}
}

void AChica::StopBreathSound()
{
	if ( BreathAudioComponent->IsPlaying() )
	{
		BreathAudioComponent->Stop();
	}
}