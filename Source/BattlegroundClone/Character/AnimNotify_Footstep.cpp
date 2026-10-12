#include "Character/AnimNotify_Footstep.h"
#include "Character/DropCharacter.h"

void UAnimNotify_Footstep::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (ADropCharacter* Character = Cast<ADropCharacter>(MeshComp ? MeshComp->GetOwner() : nullptr))
	{
		Character->PlayFootstep(MeshComp->GetSocketLocation(FootBoneName));
	}
}