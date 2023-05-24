﻿// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerObject.h"

#include "NightSkyGameState.h"
#include "NightSkyEngine/Battle/Subroutine.h"

APlayerObject::APlayerObject()
{
	Player = this;
	StoredStateMachine.Parent = this;
	MiscFlags = MISC_PushCollisionActive | MISC_WallCollisionActive | MISC_FloorCollisionActive;
	CancelFlags = CNC_EnableKaraCancel | CNC_ChainCancelEnabled; 
	PlayerFlags = PLF_DefaultLandingAction; 
	FWalkSpeed = 7800;
	BWalkSpeed = 4800;
	FDashInitSpeed = 13000;
	FDashAccel = 600;
	FDashFriction = 95;
	FDashMaxSpeed = 20000;
	BDashSpeed = 14000;
	BDashHeight = 5200;
	BDashGravity = 700;
	JumpHeight = 35000;
	FJumpSpeed = 7900;
	BJumpSpeed = 5200;
	JumpGravity = 1900;
	SuperJumpHeight = 43700;
	FSuperJumpSpeed = 7900;
	BSuperJumpSpeed = 5200;
	SuperJumpGravity = 1900;
	AirDashMinimumHeight = 105000;
	FAirDashSpeed = 30000;
	BAirDashSpeed = 24500;
	FAirDashTime = 20;
	BAirDashTime = 12;
	FAirDashNoAttackTime = 5;
	BAirDashNoAttackTime = 5;
	AirJumpCount = 1;
	AirDashCount = 1;
	Stance = ACT_Standing;
	StandPushWidth = 110000;
	StandPushHeight = 240000;
	CrouchPushWidth = 120000;
	CrouchPushHeight = 180000;
	AirPushWidth = 100000;
	AirPushHeight = 275000;
	AirPushHeightLow = -135000;
	IsPlayer = true;
	IsActive = true;
	MaxHealth = 10000;
	ForwardWalkMeterGain = 12;
	ForwardJumpMeterGain = 10;
	ForwardDashMeterGain = 25;
	ForwardAirDashMeterGain = 25;
	AttackFlags = 0;
	for (bool& Visible : ComponentVisible)
		Visible = true;
}

void APlayerObject::BeginPlay()
{
	Super::BeginPlay();
	InitPlayer();
}

void APlayerObject::InitPlayer()
{
	CurrentHealth = MaxHealth;
	StateName.SetString("Stand");
	EnableFlip(true);
	InitBP();
}

void APlayerObject::HandleStateMachine(bool Buffer)
{
	for (int i = StoredStateMachine.States.Num() - 1; i >= 0; i--)
	{
        if (!((CheckStateEnabled(StoredStateMachine.States[i]->StateType) && !StoredStateMachine.States[i]->IsFollowupState)
            || FindChainCancelOption(StoredStateMachine.States[i]->Name)
            || FindWhiffCancelOption(StoredStateMachine.States[i]->Name)
            || (CheckKaraCancel(StoredStateMachine.States[i]->StateType) && !StoredStateMachine.States[i]->IsFollowupState)
            )) //check if the state is enabled, continue if not
        {
            continue;
        }
		if (CheckObjectPreventingState(StoredStateMachine.States[i]->ObjectID)) //check if an object is preventing state entry, continue if so
		{
			continue;
		}
        //check current character state against entry state condition, continue if not entry state
		if (!StoredStateMachine.CheckStateEntryCondition(StoredStateMachine.States[i]->EntryState, Stance))
        {
            continue;
        }
		if (StoredStateMachine.States[i]->StateConditions.Num() != 0) //only check state conditions if there are any
		{
			for (int j = 0; j < StoredStateMachine.States[i]->StateConditions.Num(); j++) //iterate over state conditions
			{
                if (!HandleStateCondition(StoredStateMachine.States[i]->StateConditions[j])) //check state condition
                {
                    break;
                }
                if (j != StoredStateMachine.States[i]->StateConditions.Num() - 1) //have all conditions been met?
                {
                    continue;
                }
				for (FInputConditionList& List : StoredStateMachine.States[i]->InputConditionList)
				{
					for (int v = 0; v < List.InputConditions.Num(); v++) //iterate over input conditions
					{
						//check input condition against input buffer, if not met break.
	                    if (!StoredInputBuffer.CheckInputCondition(List.InputConditions[v]))
	                    {
	                        break;
	                    }
						if (v == List.InputConditions.Num() - 1) //have all conditions been met?
						{
							if (FindChainCancelOption(StoredStateMachine.States[i]->Name)
								|| FindWhiffCancelOption(StoredStateMachine.States[i]->Name)
								|| CancelFlags & CNC_CancelIntoSelf) //if cancel option, allow resetting state
							{
								if (Buffer)
								{
									BufferedStateName.SetString(StoredStateMachine.States[i]->Name);
									return;
								}
								if (StoredStateMachine.ForceSetState(StoredStateMachine.States[i]->Name)) //if state set successful...
								{
									StateName.SetString(StoredStateMachine.States[i]->Name);
									switch (StoredStateMachine.States[i]->EntryState)
									{
									case EEntryState::Standing:
										Stance = ACT_Standing;
										break;
									case EEntryState::Crouching:
										Stance = ACT_Crouching;
										break;
									case EEntryState::Jumping:
										Stance = ACT_Jumping;
										break;
									default:
										break;
									}
									return; //don't try to enter another state
								}
							}
							else
							{
								if (Buffer)
								{
									BufferedStateName.SetString(StoredStateMachine.States[i]->Name);
									return;
								}
								if (StoredStateMachine.SetState(StoredStateMachine.States[i]->Name)) //if state set successful...
								{
									StateName.SetString(StoredStateMachine.States[i]->Name);
									switch (StoredStateMachine.States[i]->EntryState)
									{
									case EEntryState::Standing:
										Stance = ACT_Standing;
										break;
									case EEntryState::Crouching:
										Stance = ACT_Crouching;
										break;
									case EEntryState::Jumping:
										Stance = ACT_Jumping;
										break;
									default:
										break;
									}
									return; //don't try to enter another state
								}
							}
						}
					}
					if (List.InputConditions.Num() == 0) //if no input conditions, set state
					{
						if (Buffer)
						{
							BufferedStateName.SetString(StoredStateMachine.States[i]->Name);
							return;
						}
						if (StoredStateMachine.SetState(StoredStateMachine.States[i]->Name)) //if state set successful...
						{
							StateName.SetString(StoredStateMachine.States[i]->Name);
							switch (StoredStateMachine.States[i]->EntryState)
							{
							case EEntryState::Standing:
								Stance = ACT_Standing;
								break;
							case EEntryState::Crouching:
								Stance = ACT_Crouching;
								break;
							case EEntryState::Jumping:
								Stance = ACT_Jumping;
								break;
							default:
								break;
							}
							return; //don't try to enter another state
							}
						}
					}
                continue; //this is unneeded but here for clarity.
			}
		}
		else
		{
			for (FInputConditionList& List : StoredStateMachine.States[i]->InputConditionList)
			{
				for (int v = 0; v < List.InputConditions.Num(); v++) //iterate over input conditions
				{
					//check input condition against input buffer, if not met break.
					if (!StoredInputBuffer.CheckInputCondition(List.InputConditions[v]))
					{
						break;
					}
					if (v == List.InputConditions.Num() - 1) //have all conditions been met?
					{
						if (FindChainCancelOption(StoredStateMachine.States[i]->Name)
							|| FindWhiffCancelOption(StoredStateMachine.States[i]->Name)
							|| CancelFlags & CNC_CancelIntoSelf) //if cancel option, allow resetting state
						{
							if (Buffer)
							{
								BufferedStateName.SetString(StoredStateMachine.States[i]->Name);
								return;
							}
							if (StoredStateMachine.ForceSetState(StoredStateMachine.States[i]->Name)) //if state set successful...
							{
								StateName.SetString(StoredStateMachine.States[i]->Name);
								switch (StoredStateMachine.States[i]->EntryState)
								{
								case EEntryState::Standing:
									Stance = ACT_Standing;
									break;
								case EEntryState::Crouching:
									Stance = ACT_Crouching;
									break;
								case EEntryState::Jumping:
									Stance = ACT_Jumping;
									break;
								default:
									break;
								}
								return; //don't try to enter another state
							}
						}
						else
						{
							if (Buffer)
							{
								BufferedStateName.SetString(StoredStateMachine.States[i]->Name);
								return;
							}
							if (StoredStateMachine.SetState(StoredStateMachine.States[i]->Name)) //if state set successful...
							{
								StateName.SetString(StoredStateMachine.States[i]->Name);
								switch (StoredStateMachine.States[i]->EntryState)
								{
								case EEntryState::Standing:
									Stance = ACT_Standing;
									break;
								case EEntryState::Crouching:
									Stance = ACT_Crouching;
									break;
								case EEntryState::Jumping:
									Stance = ACT_Jumping;
									break;
								default:
									break;
								}
								return; //don't try to enter another state
							}
						}
					}
				}
				if (List.InputConditions.Num() == 0) //if no input conditions, set state
				{
					if (Buffer)
					{
						BufferedStateName.SetString(StoredStateMachine.States[i]->Name);
						return;
					}
					if (StoredStateMachine.SetState(StoredStateMachine.States[i]->Name)) //if state set successful...
					{
						StateName.SetString(StoredStateMachine.States[i]->Name);
						switch (StoredStateMachine.States[i]->EntryState)
						{
						case EEntryState::Standing:
							Stance = ACT_Standing;
							break;
						case EEntryState::Crouching:
							Stance = ACT_Crouching;
							break;
						case EEntryState::Jumping:
							Stance = ACT_Jumping;
							break;
						default:
							break;
						}
						return; //don't try to enter another state
					}
				}
			}
		}
	}
}

void APlayerObject::Update()
{
	Super::Update();
	CallSubroutine("CmnOnUpdate");
	CallSubroutine("OnUpdate");

	//run input buffer before checking hitstop
	if ((Direction == DIR_Left && !Player->FlipInputs) || (Player->FlipInputs && Direction == DIR_Right)) //flip inputs with direction
	{
		const unsigned int Bit1 = Player->Inputs >> 2 & 1;
		const unsigned int Bit2 = Player->Inputs >> 3 & 1;
		unsigned int x = Bit1 ^ Bit2;

		x = x << 2 | x << 3;

		Player->Inputs = Player->Inputs ^ x;
	}

	if ((PlayerFlags & PLF_IsStunned) == 0)
	{
		Enemy->ComboCounter = 0;
		Enemy->ComboTimer = 0;
		TotalProration = 10000;
	}
	if (Inputs << 27 == 0) //if no direction, set neutral input
		Inputs |= INP_Neutral;
	else
		Inputs = Inputs & ~INP_Neutral; //remove neutral input if directional input

	if (PlayerFlags & PLF_IsThrowLock)
	{
		StoredInputBuffer.Tick(Inputs);
		HandleStateMachine(true); //handle state transitions
		if (ActionTime < ThrowTechWindow)
		{
			FInputCondition ConditionA;
			FInputBitmask BitmaskA;
			BitmaskA.InputFlag = INP_A;
			ConditionA.Sequence.Add(BitmaskA);
			ConditionA.Method = EInputMethod::Once;
			FInputCondition ConditionD;
			FInputBitmask BitmaskD;
			BitmaskD.InputFlag = INP_D;
			ConditionD.Sequence.Add(BitmaskD);
			ConditionD.Method = EInputMethod::Once;
			if ((CheckInput(ConditionA) && CheckInput(ConditionD)) || (CheckInput(ConditionD) && CheckInput(ConditionA)))
			{
				JumpToState("GuardBreak");
				Enemy->JumpToState("GuardBreak");
				PlayerFlags &= ~PLF_IsThrowLock;
				Pushback = -35000;
				Enemy->Pushback = -35000;
				HitPosX = (PosX + Enemy->PosX) / 2;
				HitPosY = (PosY + Enemy->PosY) / 2 + 250000;
				//CreateCommonParticle("cmn_throwtech", POS_Hit);
				return;
			}
		}
		return;
	}

	if (SuperFreezeTimer > 0)
	{
		StoredInputBuffer.Tick(Inputs);
		HandleStateMachine(true); //handle state transitions
		return;
	}
	if (Hitstop > 0)
	{
		StoredInputBuffer.Tick(Inputs);
		HandleStateMachine(true); //handle state transitions
		return;
	}

	if (ActionTime == 4) //enable kara cancel after window end
		CancelFlags |= CNC_EnableKaraCancel;

	if (StrikeInvulnerableTimer > 0)
		StrikeInvulnerableTimer--;
	if (ThrowInvulnerableTimer > 0)
		ThrowInvulnerableTimer--;

		
	if (PosY > GroundHeight) //set jumping if above ground
	{
		Stance = ACT_Jumping;
	}
	
	HandleBufferedState();
	
	if (ComboCounter > 0)
		ComboTimer++;
		
	if ((PlayerFlags & PLF_RoundWinInputLock) == 0)
		StoredInputBuffer.Tick(Inputs);
	else
		StoredInputBuffer.Tick(INP_Neutral);

	if (AirDashTimer > 0)
		AirDashTimer--;

	if (AirDashNoAttackTime > 0)
		AirDashNoAttackTime--;
	if (AirDashNoAttackTime == 1)
		EnableAttacks();

	if (StunTime > 0)
		StunTime--;
	if (StunTime == 1 && (PlayerFlags & PLF_IsDead) == 0)
	{
		if (StoredStateMachine.CurrentState->StateType == EStateType::Blockstun)
		{
			if (GetCurrentStateName() == "Block")
			{
				JumpToState("Stand");
			}
			else if (GetCurrentStateName() == "CrouchBlock")
			{
				JumpToState("Crouch");
			}
			else
			{
				JumpToState("VJump");
			}
		}
		else if (PosY == GroundHeight && (PlayerFlags & PLF_IsKnockedDown) == 0)
		{
			if (Stance == ACT_Standing)
			{
				JumpToState("Stand");
			}
			else if (Stance == ACT_Crouching)
			{
				JumpToState("Crouch");
			}
			TotalProration = 10000;
		}
		else
		{
			if (((PlayerFlags & PLF_IsKnockedDown) == 0 || (PlayerFlags & PLF_IsHardKnockedDown) == 0) && (PlayerFlags & PLF_IsDead) == 0)
				EnableState(ENB_Tech);
		}
	}
	
	if (StoredStateMachine.CurrentState->StateType == EStateType::Tech)
	{
		OTGCount = 0;
		ReceivedHit = FHitData();
	}

	if (PlayerFlags & PLF_TouchingWall && Enemy->StoredStateMachine.CurrentState->StateType != EStateType::Hitstun)
	{
		Enemy->Pushback = Pushback;
		Pushback = 0;
	}
		
	if (StoredStateMachine.CurrentState->StateType == EStateType::Hitstun && PosY <= GroundHeight && PrevPosY > GroundHeight)
	{
		HaltMomentum();
		if (StoredStateMachine.CurrentState->Name == "BLaunch" || StoredStateMachine.CurrentState->Name == "Blowback")
			JumpToState("FaceUpBounce");
		else if (StoredStateMachine.CurrentState->Name == "FLaunch")
			JumpToState("FaceDownBounce");
	}
	
	if (PosY <= GroundHeight && (PlayerFlags & PLF_IsKnockedDown) == 0 && ReceivedHit.GroundBounce.GroundBounceCount == 0)
	{
		if (StunTime > 0 && PrevPosY > GroundHeight)
		{
			PlayerFlags |= PLF_IsKnockedDown;
			DisableState(ENB_Tech);
		}
	}
	
	if (StoredStateMachine.CurrentState->StateType != EStateType::Hitstun)
	{
		PlayerFlags &= ~PLF_IsKnockedDown;
	}
	
	if (StoredStateMachine.CurrentState->StateType != EStateType::Hitstun)
	{
		Pushback = 0;
		PlayerFlags &= ~PLF_IsStunned;
	}
	
	if (PlayerFlags & PLF_IsKnockedDown && ActionTime == ReceivedHit.KnockdownTime && PosY <= GroundHeight && (PlayerFlags & PLF_IsDead) == 0)
	{
		Enemy->ComboCounter = 0;
		Enemy->ComboTimer = 0;
		OTGCount = 0;
		if (StoredStateMachine.CurrentState->Name == "FaceDown" || StoredStateMachine.CurrentState->Name == "FaceDownBounce")
			JumpToState("WakeUpFaceDown");
		else if (StoredStateMachine.CurrentState->Name == "FaceUp" || StoredStateMachine.CurrentState->Name == "FaceUpBounce")
			JumpToState("WakeUpFaceUp");
		TotalProration = 10000;
	}

	if (PlayerFlags & PLF_IsDead)
		DisableState(ENB_Tech);

	InstantBlockLockoutTimer--;
	if (PosY == GroundHeight && PrevPosY != GroundHeight) //reset air move counts on ground
	{
		CurrentAirJumpCount = AirJumpCount;
		CurrentAirDashCount = AirDashCount;
		if (PlayerFlags & PLF_DefaultLandingAction)
		{
			JumpToState("JumpLanding");
		}
	}
	if (GameState->BattleState.TimeUntilRoundStart <= 0)
		HandleStateMachine(false); //handle state transitions

	if (Stance == ACT_Standing) //set pushbox values based on stance
	{
		PushWidth = StandPushWidth;
		PushHeight = StandPushHeight;
		PushHeightLow = 0;
	}
	else if (Stance == ACT_Crouching)
	{
		PushWidth = CrouchPushWidth;
		PushHeight = CrouchPushHeight;
		PushHeightLow = 0;
	}
	else if (Stance == ACT_Jumping)
	{
		PushWidth = AirPushWidth;
		PushHeight = AirPushHeight;
		PushHeightLow = AirPushHeightLow;
	}

	Player->StoredStateMachine.Update();
	
	TimeUntilNextCel--;
	if (TimeUntilNextCel == 0)
		CelIndex++;
}

void APlayerObject::HandleHitAction(EHitAction HACT)
{
	for (int i = 0; i < 32; i++)
	{
		if (IsValid(ChildBattleObjects[i]))
		{
			if (ChildBattleObjects[i]->MiscFlags & MISC_DeactivateOnReceiveHit)
			{
				ChildBattleObjects[i]->DeactivateObject();
			}
		}
	}
	if (CurrentHealth <= 0)
	{
		PlayerFlags |= PLF_IsDead;
		if (PosY <= GroundHeight)
		{
			JumpToState("Crumple");
		}
		else
		{
			if (HACT == HACT_AirFaceUp)
				JumpToState("BLaunch");
			else if (HACT == HACT_AirVertical)
				JumpToState("VLaunch");
			else if (HACT == HACT_AirFaceDown)
				JumpToState("FLaunch");
			else if (HACT == HACT_Blowback)
				JumpToState("Blowback");
			else
				JumpToState("BLaunch");
		}
		ReceivedHitCommon.AttackLevel = -1;
		return;
	}
	switch (HACT)
	{
	case HACT_GroundNormal:
		if (Stance == ACT_Standing)
		{
			if (ReceivedHitCommon.AttackLevel == 0)
				JumpToState("Hitstun0");
			else if (ReceivedHitCommon.AttackLevel == 1)
				JumpToState("Hitstun1");
			else if (ReceivedHitCommon.AttackLevel == 2)
				JumpToState("Hitstun2");
			else if (ReceivedHitCommon.AttackLevel == 3)
				JumpToState("Hitstun3");
			else if (ReceivedHitCommon.AttackLevel == 4)
				JumpToState("Hitstun4");
			else if (ReceivedHitCommon.AttackLevel == 5)
				JumpToState("Hitstun5");
		}
		else if (Stance == ACT_Crouching)
		{
			if (ReceivedHitCommon.AttackLevel == 0)
				JumpToState("CrouchHitstun0");
			else if (ReceivedHitCommon.AttackLevel == 1)
				JumpToState("CrouchHitstun1");
			else if (ReceivedHitCommon.AttackLevel == 2)
				JumpToState("CrouchHitstun2");
			else if (ReceivedHitCommon.AttackLevel == 3)
				JumpToState("CrouchHitstun3");
			else if (ReceivedHitCommon.AttackLevel == 4)
				JumpToState("CrouchHitstun4");
			else if (ReceivedHitCommon.AttackLevel == 5)
				JumpToState("CrouchHitstun5");
			StunTime += 2;
		}
		break;
	case HACT_Crumple:
		JumpToState("Crumple");
		break;
	case HACT_ForceCrouch:
		Stance = ACT_Crouching;
		if (ReceivedHitCommon.AttackLevel == 0)
			JumpToState("CrouchHitstun0");
		else if (ReceivedHitCommon.AttackLevel == 1)
			JumpToState("CrouchHitstun1");
		else if (ReceivedHitCommon.AttackLevel == 2)
			JumpToState("CrouchHitstun2");
		else if (ReceivedHitCommon.AttackLevel == 3)
			JumpToState("CrouchHitstun3");
		else if (ReceivedHitCommon.AttackLevel == 4)
			JumpToState("CrouchHitstun4");
		else if (ReceivedHitCommon.AttackLevel == 5)
			JumpToState("CrouchHitstun5");
		StunTime += 2;
		break;
	case HACT_ForceStand:
		Stance = ACT_Standing;
		if (ReceivedHitCommon.AttackLevel == 0)
			JumpToState("Hitstun0");
		else if (ReceivedHitCommon.AttackLevel == 1)
			JumpToState("Hitstun1");
		else if (ReceivedHitCommon.AttackLevel == 2)
			JumpToState("Hitstun2");
		else if (ReceivedHitCommon.AttackLevel == 3)
			JumpToState("Hitstun3");
		else if (ReceivedHitCommon.AttackLevel == 4)
			JumpToState("Hitstun4");
		else if (ReceivedHitCommon.AttackLevel == 5)
			JumpToState("Hitstun5");
		break;
	case HACT_GuardBreakCrouch:
		JumpToState("GuardBreakCrouch");
		break;
	case HACT_GuardBreakStand:
		JumpToState("GuardBreak");
		break;
	case HACT_AirFaceUp:
		JumpToState("BLaunch");
		break;
	case HACT_AirVertical:
		JumpToState("VLaunch");
		break;
	case HACT_AirFaceDown:
		JumpToState("FLaunch");
		break;
	case HACT_Blowback:
		JumpToState("Blowback");
		break;
	case HACT_None: break;
	default: ;
	}
	DisableAll();
}

void APlayerObject::SetHitValues()
{
	int32 Proration = ReceivedHit.ForcedProration;
	if (Player->ComboCounter == 0)
		Proration *= ReceivedHit.InitialProration;
	else
		Proration *= 100;
	if (Player->ComboCounter == 0)
		TotalProration = 10000;
	Proration = Proration * TotalProration / 10000;
	
	if ((AttackFlags & ATK_ProrateOnce) == 0 || AttackFlags & ATK_ProrateOnce && (AttackFlags & ATK_HasHit) == 0)
		TotalProration = TotalProration * ReceivedHit.ForcedProration / 100;

	int32 FinalHitstop = ReceivedHitCommon.Hitstop + ReceivedHit.EnemyHitstopModifier;
	
	Enemy->Hitstop = ReceivedHitCommon.Hitstop;
	Hitstop = FinalHitstop;

	int FinalDamage;
	if (Player->ComboCounter == 0)
		FinalDamage = ReceivedHit.Damage;
	else
		FinalDamage = ReceivedHit.Damage * Proration * Player->ComboRate / 1000000;

	if (FinalDamage < ReceivedHit.MinimumDamagePercent * ReceivedHit.Damage / 100)
		FinalDamage = ReceivedHit.Damage * ReceivedHit.MinimumDamagePercent / 100;

	CurrentHealth -= FinalDamage;

	const int32 FinalHitPushbackX = ReceivedHit.GroundPushbackX + Player->ComboCounter * ReceivedHit.GroundPushbackX / 60;
	const int32 FinalAirHitPushbackX = ReceivedHit.AirPushbackX + Player->ComboCounter * ReceivedHit.AirPushbackX / 60;
	const int32 FinalAirHitPushbackY = ReceivedHit.AirPushbackY - Player->ComboCounter * ReceivedHit.AirPushbackY / 120;
	const int32 FinalGravity = ReceivedHit.Gravity - Player->ComboCounter * ReceivedHit.Gravity / 60;
	
	EHitAction HACT;
	
	if (PosY == GroundHeight && !(PlayerFlags & PLF_IsKnockedDown))
		HACT = ReceivedHit.GroundHitAction;
	else
		HACT = ReceivedHit.AirHitAction;
	
	switch (HACT)
	{
	case HACT_GroundNormal:
	case HACT_ForceCrouch:
	case HACT_ForceStand:
		StunTime = ReceivedHit.Hitstun;
		Pushback = -FinalHitPushbackX;
		if (PlayerFlags & PLF_TouchingWall)
		{
			Enemy->Pushback = -FinalHitPushbackX;
		}
		break;
	case HACT_AirFaceUp:
	case HACT_AirVertical:
	case HACT_AirFaceDown:
		StunTime = ReceivedHit.Untech;
		SpeedX = -FinalAirHitPushbackX;
		SpeedY = FinalAirHitPushbackY;
		Gravity = FinalGravity;
		if (PlayerFlags & PLF_TouchingWall)
		{
			Enemy->Pushback = -ReceivedHit.GroundPushbackX;
		}
		break;
	case HACT_Blowback:
		StunTime = ReceivedHit.Untech;
		SpeedX = -FinalAirHitPushbackX * 3 / 2;
		SpeedY = FinalAirHitPushbackY * 3 / 2;
		Gravity = FinalGravity;
		if (PlayerFlags & PLF_TouchingWall)
		{
			Enemy->Pushback = -ReceivedHit.GroundPushbackX;
		}
	default:
		break;
	}
	AirDashTimer = 0;
	AirDashNoAttackTime = 0;
}

bool APlayerObject::IsCorrectBlock(EBlockType BlockType)
{
	if (BlockType != BLK_None)
	{
		FInputCondition Left;
		FInputBitmask BitmaskLeft;
		BitmaskLeft.InputFlag = INP_Left;
		Left.Sequence.Add(BitmaskLeft);
		Left.bInputAllowDisable = false;
		Left.Lenience = 12;
		FInputCondition Right;
		FInputBitmask BitmaskRight;
		BitmaskRight.InputFlag = INP_Right;
		Right.Sequence.Add(BitmaskRight);
		if (CheckInput(Left) && !CheckInput(Right) && PosY > GroundHeight || GetCurrentStateName() == "AirBlock")
		{
			Left.Method = EInputMethod::Once;
			if (CheckInput(Left) && InstantBlockLockoutTimer == 0)
			{
				//AddMeter(800);
			}
			return true;
		}
		FInputCondition Input1;
		FInputBitmask BitmaskDownLeft;
		BitmaskDownLeft.InputFlag = INP_DownLeft;
		Input1.Sequence.Add(BitmaskDownLeft);
		Input1.Method = EInputMethod::Strict;
		Input1.bInputAllowDisable = false;
		Input1.Lenience = 12;
		if ((CheckInput(Input1) || GetCurrentStateName() == "CrouchBlock") && BlockType != BLK_High && !CheckInput(Right))
		{
			Input1.Method = EInputMethod::OnceStrict;
			if (CheckInput(Input1) && InstantBlockLockoutTimer == 0)
			{
				//AddMeter(800);
			}
			return true;
		}
		FInputCondition Input4;
		Input4.Sequence.Add(BitmaskLeft);
		Input4.Method = EInputMethod::Strict;
		Input4.bInputAllowDisable = false;
		Input4.Lenience = 12;
		if ((CheckInput(Input4) || GetCurrentStateName() == "Block") && BlockType != BLK_Low && !CheckInput(Right))
		{
			Input4.Method = EInputMethod::OnceStrict;
			if (CheckInput(Input4) && InstantBlockLockoutTimer == 0)
			{
				//AddMeter(800);
			}
			return true;
		}
	}
	return false;
}

void APlayerObject::HandleBlockAction(EBlockType BlockType)
{
	FInputCondition Input1;
	FInputBitmask BitmaskDownLeft;
	BitmaskDownLeft.InputFlag = INP_DownLeft;
	Input1.Sequence.Add(BitmaskDownLeft);
	Input1.Method = EInputMethod::Strict;
	FInputCondition Left;
	FInputBitmask BitmaskLeft;
	BitmaskLeft.InputFlag = INP_Left;
	Left.Sequence.Add(BitmaskLeft);
	if ((CheckInput(Left) && PosY > GroundHeight) || GetCurrentStateName() == "AirBlock")
	{
		JumpToState("AirBlock");
		Stance = ACT_Jumping;
	}
	else if ((CheckInput(Input1) && PosY <= GroundHeight) || GetCurrentStateName() == "CrouchBlock")
	{
		JumpToState("CrouchBlock");
		Stance = ACT_Crouching;
	}
	else 
	{
		JumpToState("Block");
		Stance = ACT_Standing;
	}
}

void APlayerObject::EmptyStateMachine()
{
	StoredStateMachine.States.Empty();
	StoredStateMachine.StateNames.Empty();
	StoredStateMachine.CurrentState = nullptr;
}

void APlayerObject::HandleBufferedState()
{
	if (strcmp(BufferedStateName.GetString(), ""))
	{
		if (FindChainCancelOption(BufferedStateName.GetString())
			|| FindWhiffCancelOption(BufferedStateName.GetString())) //if cancel option, allow resetting state
		{
			if (StoredStateMachine.ForceSetState(BufferedStateName.GetString()))
			{
				StateName.SetString(BufferedStateName.GetString());
				switch (StoredStateMachine.CurrentState->EntryState)
				{
				case EEntryState::Standing:
					Stance = ACT_Standing;
					break;
				case EEntryState::Crouching:
					Stance = ACT_Crouching;
					break;
				case EEntryState::Jumping:
					Stance = ACT_Jumping;
					break;
				default:
					break;
				}
			}
			BufferedStateName.SetString("");
		}
		else
		{
			if (StoredStateMachine.SetState(BufferedStateName.GetString()))
			{
				StateName.SetString(BufferedStateName.GetString());
				switch (StoredStateMachine.CurrentState->EntryState)
				{
				case EEntryState::Standing:
					Stance = ACT_Standing;
					break;
				case EEntryState::Crouching:
					Stance = ACT_Crouching;
					break;
				case EEntryState::Jumping:
					Stance = ACT_Jumping;
					break;
				default:
					break;
				}
			}
			BufferedStateName.SetString("");
		}
	}
}

bool APlayerObject::HandleStateCondition(EStateCondition StateCondition)
{
	switch(StateCondition)
	{
	case EStateCondition::None:
		ReturnReg = true;
	case EStateCondition::AirJumpOk:
		if (CurrentAirJumpCount > 0)
			ReturnReg = true;
		break;
	case EStateCondition::AirJumpMinimumHeight:
		if (SpeedY <= 0 || PosY >= 122500)
			ReturnReg = true;
		break;
	case EStateCondition::AirDashOk:
		if (CurrentAirDashCount > 0)
			ReturnReg = true;
		break;
	case EStateCondition::AirDashMinimumHeight:
		if (PosY > AirDashMinimumHeight && SpeedY > 0)
			ReturnReg = true;
		if (PosY > 70000 && SpeedY <= 0)
			ReturnReg = true;
		break;
	case EStateCondition::IsAttacking:
		ReturnReg = AttackFlags & ATK_IsAttacking;
	case EStateCondition::HitstopCancel:
		ReturnReg = Hitstop == 0 && AttackFlags & ATK_IsAttacking;
	case EStateCondition::IsStunned:
		ReturnReg = PlayerFlags & PLF_IsStunned;
	case EStateCondition::CloseNormal:
		if (abs(PosX - Enemy->PosX) < CloseNormalRange && (PlayerFlags & PLF_ForceEnableFarNormal) == 0)
			ReturnReg = true;
		break;
	case EStateCondition::FarNormal:
		if (abs(PosX - Enemy->PosX) > CloseNormalRange || PlayerFlags & PLF_ForceEnableFarNormal)
			ReturnReg = true;
		break;
	case EStateCondition::MeterNotZero:
		if (GameState->BattleState.Meter[PlayerIndex] > 0)
			ReturnReg = true;
		break;
	case EStateCondition::MeterQuarterBar:
		if (GameState->BattleState.Meter[PlayerIndex] >= 2500)
			ReturnReg = true;
		break;
	case EStateCondition::MeterHalfBar:
		if (GameState->BattleState.Meter[PlayerIndex] >= 5000)
			ReturnReg = true;
		break;
	case EStateCondition::MeterOneBar:
		if (GameState->BattleState.Meter[PlayerIndex] >= 10000)
			ReturnReg = true;
		break;
	case EStateCondition::MeterTwoBars:
		if (GameState->BattleState.Meter[PlayerIndex] >= 20000)
			ReturnReg = true;
		break;
	case EStateCondition::MeterThreeBars:
		if (GameState->BattleState.Meter[PlayerIndex] >= 30000)
			ReturnReg = true;
		break;
	case EStateCondition::MeterFourBars:
		if (GameState->BattleState.Meter[PlayerIndex] >= 40000)
			ReturnReg = true;
		break;
	case EStateCondition::MeterFiveBars:
		if (GameState->BattleState.Meter[PlayerIndex] >= 50000)
			ReturnReg = true;
		break;
	case EStateCondition::PlayerReg1True:
		ReturnReg = PlayerReg1 != 0;
	case EStateCondition::PlayerReg2True:
		ReturnReg = PlayerReg2 != 0;
	case EStateCondition::PlayerReg3True:
		ReturnReg = PlayerReg3 != 0;
	case EStateCondition::PlayerReg4True:
		ReturnReg = PlayerReg4 != 0;
	case EStateCondition::PlayerReg5True:
		ReturnReg = PlayerReg5 != 0;
	case EStateCondition::PlayerReg6True:
		ReturnReg = PlayerReg6 != 0;
	case EStateCondition::PlayerReg7True:
		ReturnReg = PlayerReg7 != 0;
	case EStateCondition::PlayerReg8True:
		ReturnReg = PlayerReg8 != 0;
	case EStateCondition::PlayerReg1False:
		ReturnReg = PlayerReg1 == 0;
	case EStateCondition::PlayerReg2False:
		ReturnReg = PlayerReg2 == 0;
	case EStateCondition::PlayerReg3False:
		ReturnReg = PlayerReg3 == 0;
	case EStateCondition::PlayerReg4False:
		ReturnReg = PlayerReg4 == 0;
	case EStateCondition::PlayerReg5False:
		ReturnReg = PlayerReg5 == 0;
	case EStateCondition::PlayerReg6False:
		ReturnReg = PlayerReg6 == 0;
	case EStateCondition::PlayerReg7False:
		ReturnReg = PlayerReg7 == 0;
	case EStateCondition::PlayerReg8False:
		ReturnReg = PlayerReg8 == 0;
	default:
		ReturnReg = false;
	}
	return ReturnReg;
}

bool APlayerObject::FindChainCancelOption(const FString& Name)
{
	if (AttackFlags & ATK_HasHit && AttackFlags & ATK_IsAttacking && CancelFlags & CNC_ChainCancelEnabled)
	{
		for (int i = 0; i < CancelArraySize; i++)
		{
			if (ChainCancelOptionsInternal[i] == StoredStateMachine.GetStateIndex(Name) && ChainCancelOptionsInternal[i] != INDEX_NONE)
			{
				ReturnReg = true;
			}
		}
	}
	else
	{
		ReturnReg = false;
	}
	return ReturnReg;
}

bool APlayerObject::FindWhiffCancelOption(const FString& Name)
{
	if (CancelFlags & CNC_WhiffCancelEnabled)
	{
		for (int i = 0; i < CancelArraySize; i++)
		{
			if (WhiffCancelOptionsInternal[i] == StoredStateMachine.GetStateIndex(Name) && WhiffCancelOptionsInternal[i] != INDEX_NONE)
			{
				ReturnReg = true;
			}
		}
	}
	else
	{
		ReturnReg = false;
	}
	return ReturnReg;
}

bool APlayerObject::CheckKaraCancel(EStateType InStateType)
{
	if ((CancelFlags & CNC_EnableKaraCancel) == 0)
	{
		ReturnReg = false;
		return ReturnReg;
	}
	
	CancelFlags &= ~CNC_EnableKaraCancel; //prevents kara cancelling immediately after the last kara cancel
	
	//two checks: if it's an attack, and if the given state type has a higher or equal priority to the current state
	if (InStateType == EStateType::NormalThrow && StoredStateMachine.CurrentState->StateType < InStateType
		&& StoredStateMachine.CurrentState->StateType >= EStateType::NormalAttack && ActionTime < 3
		&& ComboTimer == 0)
	{
		ReturnReg = true;
	}
	if (InStateType == EStateType::SpecialAttack && StoredStateMachine.CurrentState->StateType < InStateType
		&& StoredStateMachine.CurrentState->StateType >= EStateType::NormalAttack && ActionTime < 3)
	{
		ReturnReg = true;
	}
	if (InStateType == EStateType::SuperAttack && StoredStateMachine.CurrentState->StateType < InStateType
		&& StoredStateMachine.CurrentState->StateType >= EStateType::SpecialAttack && ActionTime < 3)
	{
		ReturnReg = true;
	}	
	return ReturnReg;
}

bool APlayerObject::CheckObjectPreventingState(int InObjectID)
{
	ReturnReg = false;
	for (int i = 0; i < 32; i++)
	{
		if (IsValid(ChildBattleObjects[i]))
		{
			if (ChildBattleObjects[i]->IsActive)
			{
				if (ChildBattleObjects[i]->ObjectID == InObjectID && ChildBattleObjects[i]->ObjectID != 0)
					ReturnReg = true;
			}
		}
	}
	return ReturnReg;
}

void APlayerObject::AddState(FString Name, UState* State)
{
	StoredStateMachine.Parent = this;
	StoredStateMachine.AddState(Name, State);
}

void APlayerObject::AddSubroutine(FString Name, USubroutine* Subroutine)
{
	Subroutine->Parent = this;
	Subroutines.Add(Subroutine);
	SubroutineNames.Add(Name);
}

void APlayerObject::CallSubroutine(FString Name)
{
	if (CommonSubroutineNames.Find(Name) != INDEX_NONE)
	{
		CommonSubroutines[CommonSubroutineNames.Find(Name)]->Exec();
		return;
	}

	if (SubroutineNames.Find(Name) != INDEX_NONE)
		Subroutines[SubroutineNames.Find(Name)]->Exec();
}

void APlayerObject::SetStance(EActionStance InStance)
{
	Stance = InStance;
}

void APlayerObject::JumpToState(FString NewName)
{
	if (StoredStateMachine.ForceSetState(NewName))
		StateName.SetString(NewName);
	if (StoredStateMachine.CurrentState != nullptr)
	{
		switch (StoredStateMachine.CurrentState->EntryState)
		{
		case EEntryState::Standing:
			Stance = ACT_Standing;
			break;
		case EEntryState::Crouching:
			Stance = ACT_Crouching;
			break;
		case EEntryState::Jumping:
			Stance = ACT_Jumping;
			break;
		default:
			break;
		}
	}
}

FString APlayerObject::GetCurrentStateName() const
{
	return StoredStateMachine.CurrentState->Name;
}

bool APlayerObject::CheckStateEnabled(EStateType StateType)
{
	switch (StateType)
	{
	case EStateType::Standing:
		if (EnableFlags & ENB_Standing)
			ReturnReg = true;
		break;
	case EStateType::Crouching:
		if (EnableFlags & ENB_Crouching)
			ReturnReg = true;
		break;
	case EStateType::NeutralJump:
	case EStateType::ForwardJump:
	case EStateType::BackwardJump:
		if (EnableFlags & ENB_Jumping || (CancelFlags & CNC_JumpCancel && AttackFlags & ATK_HasHit && AttackFlags & ATK_IsAttacking))
			ReturnReg = true;
		break;
	case EStateType::ForwardWalk:
		if (EnableFlags & ENB_ForwardWalk)
			ReturnReg = true;
		break;
	case EStateType::BackwardWalk:
		if (EnableFlags & ENB_BackWalk)
			ReturnReg = true;
		break;
	case EStateType::ForwardDash:
		if (EnableFlags & ENB_ForwardDash)
			ReturnReg = true;
		break;
	case EStateType::BackwardDash:
		if (EnableFlags & ENB_BackDash)
			ReturnReg = true;
		break;
	case EStateType::ForwardAirDash:
		if (EnableFlags & ENB_ForwardAirDash || (CancelFlags & CNC_FAirDashCancel && AttackFlags & ATK_HasHit && AttackFlags & ATK_IsAttacking))
			ReturnReg = true;
		break;
	case EStateType::BackwardAirDash:
		if (EnableFlags & ENB_BackAirDash || (CancelFlags & CNC_BAirDashCancel && AttackFlags & ATK_HasHit && AttackFlags & ATK_IsAttacking))
			ReturnReg = true;
		break;
	case EStateType::NormalAttack:
	case EStateType::NormalThrow:
		if (EnableFlags & ENB_NormalAttack)
			ReturnReg = true;
		break;
	case EStateType::SpecialAttack:
		if (EnableFlags & ENB_SpecialAttack || (CancelFlags & CNC_SpecialCancel && AttackFlags & ATK_HasHit && AttackFlags & ATK_IsAttacking))
			ReturnReg = true;
		break;
	case EStateType::SuperAttack:
		if (EnableFlags & ENB_SuperAttack || (CancelFlags & CNC_SuperCancel && AttackFlags & ATK_HasHit && AttackFlags & ATK_IsAttacking))
			ReturnReg = true;
		break;
	case EStateType::Tech:
		if (EnableFlags & ENB_Tech)
			ReturnReg = true;
		break;
	case EStateType::Burst:
		if (Enemy->Player->PlayerFlags & PLF_LockOpponentBurst && (PlayerFlags & PLF_IsDead) == 0)
			ReturnReg = true;
		break;
	default:
		ReturnReg = false;
	}
	return ReturnReg;
}

void APlayerObject::OnStateChange()
{
	// Deactivate all objects that need to be destroyed on state change.
	for (int i = 0; i < 32; i++)
	{
		if (IsValid(ChildBattleObjects[i]))
		{
			if (ChildBattleObjects[i]->MiscFlags & MISC_DeactivateOnStateChange)
			{
				ChildBattleObjects[i]->DeactivateObject();
			}
		}
	}
	
	DisableLastInput();
	DisableAll();
	
	// Reset flags
	CancelFlags = CNC_EnableKaraCancel | CNC_ChainCancelEnabled; 
	PlayerFlags &= ~PLF_ThrowActive;
	PlayerFlags &= ~PLF_DeathCamOverride;
	PlayerFlags &= ~PLF_LockOpponentBurst;
	PlayerFlags &= ~PLF_ForceEnableFarNormal;
	PlayerFlags |= PLF_DefaultLandingAction;
	AttackFlags &= ~ATK_IsAttacking;
	AttackFlags &= ~ATK_ProrateOnce;
	AttackFlags &= ~ATK_AttackHeadAttribute;
	AttackFlags &= ~ATK_AttackProjectileAttribute;
	AttackFlags &= ~ATK_HasHit;
	MiscFlags = 0;
	MiscFlags |= MISC_PushCollisionActive;
	MiscFlags |= MISC_WallCollisionActive;
	MiscFlags |= MISC_FloorCollisionActive;
	MiscFlags |= MISC_InertiaEnable;
	InvulnFlags = 0;

	// Reset speed modifiers
	SpeedXRate = 100;
	SpeedXRatePerFrame = 100;
	SpeedYRate = 100;
	SpeedYRatePerFrame = 100;

	// Reset action data
	ActionTime = 0;
	AnimFrame = 0;
	CelIndex = 0;
	TimeUntilNextCel = 0;
	for (auto& Handler : EventHandlers)
		Handler = FEventHandler();
	EventHandlers[EVT_Enter].FunctionName.SetString("Init");	

	// Reset action registers
	ActionReg1 = 0;
	ActionReg2 = 0;
	ActionReg3 = 0;
	ActionReg4 = 0;
	ActionReg5 = 0;
	ActionReg6 = 0;
	ActionReg7 = 0;
	ActionReg8 = 0;

	// Miscellaneous resets
	PushWidthFront = 0;
	FlipInputs = false;
	ChainCancelOptions.Empty();
	WhiffCancelOptions.Empty();
	HitCommon = FHitDataCommon();
	NormalHit = FHitData();
	CounterHit = FHitData();
	AnimName.SetString("");
	AnimFrame = 0;
	CelName.SetString("");
}

void APlayerObject::ResetForRound()
{
	if (PlayerIndex == 0)
	{
		PosX = -360000;
	}
	else
	{
		PosX = 360000;
	}
	PosY = 0;
	PosZ = 0;
	PrevPosX = 0;
	PrevPosY = 0;
	PrevPosZ = 0;
	SpeedX = 0;
	SpeedY = 0;
	SpeedZ = 0;
	Gravity = 1900;
	Inertia = 0;
	Pushback = 0;
	ActionTime = 0;
	PushHeight = 0;
	PushHeightLow = 0;
	PushWidth = 0;
	PushWidthFront  = 0;
	Hitstop = 0;
	L = 0;
	R = 0;
	T = 0;
	B = 0;
	HitCommon = FHitDataCommon();
	NormalHit = FHitData();
	CounterHit = FHitData();
	ReceivedHitCommon = FHitDataCommon();
	ReceivedHit = FHitData();
	AttackFlags = 0;
	StunTime = 0;
	Hitstop = 0;
	MiscFlags = 0;
	MiscFlags |= MISC_PushCollisionActive;
	MiscFlags |= MISC_WallCollisionActive;
	MiscFlags |= MISC_FloorCollisionActive;
	MiscFlags |= MISC_InertiaEnable;
	Direction = DIR_Right;
	SpeedXRate = 100;
	SpeedXRatePerFrame = 100;
	SpeedYRate = 100;
	SpeedYRatePerFrame = 100;
	SpeedZRate = 100;
	SpeedZRatePerFrame = 100;
	GroundHeight = 0;
	ReturnReg = 0;
	ActionReg1 = 0;
	ActionReg2 = 0;
	ActionReg3 = 0;
	ActionReg4 = 0;
	ActionReg5 = 0;
	ActionReg6 = 0;
	ActionReg7 = 0;
	ActionReg8 = 0;
	IsPlayer = true;
	SuperFreezeTimer = 0;
	CelName.SetString("");
	AnimName.SetString("");
	AnimFrame = 0;
	CelIndex = 0;
	TimeUntilNextCel = 0;
	for (auto& Handler : EventHandlers)
		Handler = FEventHandler();
	EventHandlers[EVT_Enter].FunctionName.SetString("Init");
	HitPosX = 0;
	HitPosY = 0;
	for (auto& Box : Boxes)
	{
		Box = FCollisionBox();
	}
	PlayerReg1 = 0;
	PlayerReg2 = 0;
	PlayerReg3 = 0;
	PlayerReg4 = 0;
	PlayerReg5 = 0;
	PlayerReg6 = 0;
	PlayerReg7 = 0;
	PlayerReg8 = 0;
	Inputs = 0;
	FlipInputs = false;
	Stance = ACT_Standing;
	CurrentHealth = MaxHealth;
	TotalProration = 10000;
	ComboCounter = 0;
	ComboTimer = 0;
	ThrowTechWindow = 6;
	InvulnFlags = 0;
	PlayerFlags &= ~PLF_IsDead;
	PlayerFlags &= ~PLF_ThrowActive;
	PlayerFlags &= ~PLF_IsStunned;
	PlayerFlags &= ~PLF_IsThrowLock;
	PlayerFlags &= ~PLF_DeathCamOverride;
	PlayerFlags &= ~PLF_IsKnockedDown;
	PlayerFlags &= ~PLF_RoundWinInputLock;
	PlayerFlags &= ~PLF_LockOpponentBurst;
	PlayerFlags &= ~PLF_ForceEnableFarNormal;
	PlayerFlags |= PLF_DefaultLandingAction;
	StrikeInvulnerableTimer = 0;
	ThrowInvulnerableTimer = 0;
	for (auto& Gauge : ExtraGauges)
		Gauge.Value = Gauge.InitialValue;
	AirDashTimer = 0;
	OTGCount = 0;
	for (auto& ChildObj : ChildBattleObjects)
		ChildObj = nullptr;
	for (auto& StoredObj : StoredBattleObjects)
		StoredObj = nullptr;
	SavedInputCondition = FSavedInputCondition();
	CurrentAirJumpCount = 0;
	CurrentAirDashCount = 0;
	AirDashTimerMax = 0;
	CancelFlags = 0;
	EnableFlags = 0;
	AirDashNoAttackTime = 0;
	InstantBlockLockoutTimer = 0;
	MeterCooldownTimer = 0;
	for (int32& CancelOption : ChainCancelOptionsInternal)
		CancelOption = 0;
	for (int32& CancelOption : WhiffCancelOptionsInternal)
		CancelOption = 0;
	ExeStateName.SetString("");
	BufferedStateName.SetString("");
	JumpToState("Stand");
}

void APlayerObject::DisableLastInput()
{
	StoredInputBuffer.InputDisabled[89] = StoredInputBuffer.InputBufferInternal[89];
}

void APlayerObject::EnableState(EEnableFlags EnableType)
{
	EnableFlags |= EnableType;
}

void APlayerObject::DisableState(EEnableFlags EnableType)
{
	EnableFlags = EnableFlags & ~EnableType;
}

void APlayerObject::EnableAttacks()
{
	EnableState(ENB_NormalAttack);
	EnableState(ENB_SpecialAttack);
	EnableState(ENB_SuperAttack);
}

void APlayerObject::EnableAll()
{
	EnableState(ENB_Standing);
	EnableState(ENB_Crouching);
	EnableState(ENB_Jumping);
	EnableState(ENB_ForwardWalk);
	EnableState(ENB_BackWalk);
	EnableState(ENB_ForwardDash);
	EnableState(ENB_BackDash);
	EnableState(ENB_ForwardAirDash);
	EnableState(ENB_BackAirDash);
	EnableState(ENB_NormalAttack);
	EnableState(ENB_SpecialAttack);
	EnableState(ENB_SuperAttack);
	EnableState(ENB_Block);
}

void APlayerObject::DisableAll()
{
	DisableState(ENB_Standing);
	DisableState(ENB_Crouching);
	DisableState(ENB_Jumping);
	DisableState(ENB_ForwardWalk);
	DisableState(ENB_BackWalk);
	DisableState(ENB_ForwardDash);
	DisableState(ENB_BackDash);
	DisableState(ENB_ForwardAirDash);
	DisableState(ENB_BackAirDash);
	DisableState(ENB_NormalAttack);
	DisableState(ENB_SpecialAttack);
	DisableState(ENB_SuperAttack);
	DisableState(ENB_Block);
}

bool APlayerObject::CheckInputRaw(EInputFlags Input)
{
	if (Inputs & Input)
	{
		ReturnReg = true;
	}
	else
	{
		ReturnReg = false;
	}
	return ReturnReg;
}

bool APlayerObject::CheckInput(const FInputCondition& Input)
{
	ReturnReg = StoredInputBuffer.CheckInputCondition(Input);
	return ReturnReg;
}

void APlayerObject::AddAirJump(int32 NewAirJump)
{
	CurrentAirJumpCount += NewAirJump;
}

void APlayerObject::AddAirDash(int32 NewAirDash)
{
	CurrentAirDashCount += NewAirDash;
}

void APlayerObject::SetAirDashTimer(bool IsForward)
{
	if (IsForward)
		AirDashTimer = FAirDashTime + 1;
	else
		AirDashTimer = BAirDashTime + 1;
}

void APlayerObject::SetAirDashNoAttackTimer(bool IsForward)
{
	if (IsForward)
		AirDashNoAttackTime = FAirDashNoAttackTime + 1;
	else
		AirDashNoAttackTime = BAirDashNoAttackTime + 1;
}

void APlayerObject::AddChainCancelOption(FString Option)
{
	ChainCancelOptions.Add(Option);
	if (ChainCancelOptions.Num() > 0)
	{
		ChainCancelOptionsInternal[ChainCancelOptions.Num() - 1] = StoredStateMachine.GetStateIndex(Option);
	}
}

void APlayerObject::AddWhiffCancelOption(FString Option)
{
	WhiffCancelOptions.Add(Option);
	if (WhiffCancelOptions.Num() > 0)
	{
		WhiffCancelOptionsInternal[WhiffCancelOptions.Num() - 1] = StoredStateMachine.GetStateIndex(Option);
	}
}

void APlayerObject::SetDefaultLandingAction(bool Enable)
{
	if (Enable)
	{
		PlayerFlags |= PLF_DefaultLandingAction;
	}
	else
	{
		PlayerFlags &= ~PLF_DefaultLandingAction;
	}
}
