#include "RealGuitarGameMode.h"

#include "AcousticChillCharacter.h"

ARealGuitarGameMode::ARealGuitarGameMode()
{
    DefaultPawnClass = AAcousticChillCharacter::StaticClass();
}
