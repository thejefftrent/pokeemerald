#include "global.h"
#include "data.h"
#include "strings.h"
#include "ce/rival_names.h"
#include "constants/trainers.h"

const u8 *CeRivalNames_OpponentName(u16 trainerId, const u8 *name)
{
    if (IsPartnerTrainerId(trainerId) || GetTrainerClassFromId(trainerId) != TRAINER_CLASS_RIVAL)
        return name;

    switch (GetTrainerPicFromId(trainerId))
    {
    case TRAINER_PIC_BRENDAN:
        return gText_ExpandedPlaceholder_Brendan;
    case TRAINER_PIC_MAY:
        return gText_ExpandedPlaceholder_May;
    default:
        return name;
    }
}
