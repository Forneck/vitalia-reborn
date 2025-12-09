# Zone 98 Triggers Documentation

## Overview
This document describes the DG Script triggers implemented for Zone 98 (O Abismo) to replace the old C-based special procedures for the angel quest and evil boss mechanics.

## Implemented Triggers

### Angel Triggers (9803-9806)

These triggers implement the angel greeting system where four angels guide players through a quest.

#### Trigger 9803: Angel 9806 Greet (Anjo da Luz)
- **Type**: Mob Greet Trigger
- **Attached to**: Mob 9806 (o Anjo da Luz)
- **Description**: Greets visible players (not NPCs or invisible immortals) with the first test message
- **Message**: "Você passou pelo primeiro teste, a cada passagem saiba guardar os ensinamentos, pois com eles você derrotará o Mal Supremo. Agora siga e trace seu destino."

#### Trigger 9804: Angel 9807 Greet (Anjo do Amor)
- **Type**: Mob Greet Trigger
- **Attached to**: Mob 9807 (o Anjo do Amor)
- **Description**: Greets visible players with wisdom about love magic
- **Message**: "Sabia foi sua escolha, assim como sábio você deve ser. Para poder derrotar o inimigo maior, deve adquirir aliados também poderosos. Lembre-se com a magia do amor seus inimigos serão os mais fiéis soldados."

#### Trigger 9805: Angel 9808 Greet (Anjo da Honestidade)
- **Type**: Mob Greet Trigger
- **Attached to**: Mob 9808 (o Anjo da Honestidade)
- **Description**: Greets visible players with instructions about using the love enchantment
- **Message**: "Vitalia está em festa, pois o grande aventureiro se mostra digno. Quando estiver diante dos maléficos poderes use o encantamento do amor, para que o mal se transforme em amigo e juntos vão em busca de libertar a legião do bem, pois lá receberá o poder de por os maus contra os maus, e só assim o Mal Supremo será derrotado."

#### Trigger 9806: Angel 9809 Greet and Give Scroll (Anjo da Vitória)
- **Type**: Mob Greet Trigger
- **Attached to**: Mob 9809 (o Anjo da Vitória)
- **Description**: Greets visible players and gives them the sacred scroll (object 9808)
- **Message**: "Abençoado seja você aventureiro de Vitalia, que com honestidade e amor chegou até aqui, receba esse poderoso pergaminho escritos pelos Deuses de Vitalia. Use-o com fé e seus mais fortes inimigos lhe serão os servos mais fiéis. Agora dê um passo atrás para que seu destino seja traçado."
- **Action**: Gives "pergaminho dourado" (scroll 9808) to the player if the angel has it in inventory

### Evil Boss Triggers (9807-9808)

These triggers implement the boss fight mechanics where charmed mobs assist the player.

#### Trigger 9807: Evil Boss Fight - Charmed Allies
- **Type**: Mob Kill Trigger (fires when mob enters combat)
- **Attached to**: Mob 9804 (o Mal Supremo)
- **Description**: When the evil boss enters combat, any charmed mobs (9801-9803) in the room will attack the boss alongside their master
- **Mechanics**:
  - Checks all characters in the room
  - Identifies charmed mobs with vnums 9801 (Cérbero), 9802 (Ciclope gigante), or 9803 (Monstro de Pedra)
  - If they have a master and aren't already fighting, forces them to attack the boss
  - Mobs say: "Lutarei ao lado de (meu mestre/minha mestra)!"

#### Trigger 9808: Evil Boss Death - Free Charmed Mobs
- **Type**: Mob Death Trigger (fires when mob dies)
- **Attached to**: Mob 9804 (o Mal Supremo)
- **Description**: When the evil boss dies, all charmed special mobs are freed and removed from the world
- **Mechanics**:
  - Loops through all mobs in the entire world
  - Finds any charmed mobs with vnums 9801, 9802, or 9803
  - Each mob says: "Minha missão aqui terminou."
  - Then says: "(mob name) desaparece misteriosamente."
  - Purges the mob from the game

### Room Trigger (9809)

#### Trigger 9809: Room 9823 Entry Message
- **Type**: Room Greet Trigger
- **Attached to**: Room 9823 (O Calabouço dos Anjos)
- **Description**: When a player enters room 9823, if scroll 9808 is present in the room, displays a liberation message
- **Message**: "Alguém lhe diz, 'Louvados sejam os Deuses de Vitalia por nos libertar. Em nome da honra e da bondade lhe damos esse pergaminho que há de lhe ajudar a derrotar o mal que impera nesse lugar."

### Object Trigger (9810)

#### Trigger 9810: Scroll 9808 Use Command
- **Type**: Object Command Trigger
- **Attached to**: Object 9808 (pergaminho dourado)
- **Commands**: usar, use, ativar, activate
- **Description**: Activates the scroll to charm one of the three special guardian mobs
- **Mechanics**:
  1. Player must specify a target: "usar pergaminho <target>"
  2. Validates the target exists in the room
  3. Cannot be used on self or other players
  4. If target is mob 9801, 9802, or 9803:
     - Wakes up the mob if sleeping
     - Forces the mob to follow the player
     - Applies charm affect (duration: 999)
     - Mob says: "Reconheço você como (meu senhor/minha senhora), e irei ajudar-lhe em sua batalha."
     - Scroll is destroyed
  5. If target is any other mob, shows "O pergaminho não tem efeito."

## Quest Flow

1. Player enters zone 98 and encounters the four angels (9806-9809)
2. Each angel provides guidance about the quest
3. The final angel (9809) gives the player the sacred scroll (9808)
4. Player must find and charm the three guardian mobs using the scroll:
   - Cérbero (9801)
   - Ciclope gigante (9802)
   - Monstro de Pedra (9803)
5. With these charmed allies, the player can fight the evil boss (9804)
6. When combat starts, charmed mobs automatically assist the player
7. When the boss dies, all charmed mobs are freed and removed

## Technical Notes

- All triggers check for player visibility (not NPCs or invisible immortals)
- The charm effect duration is set to 999 ticks for persistence
- The boss fight trigger loops through room occupants to find charmed allies
- The boss death trigger loops through all global mobs to clean up
- Scroll is consumed on successful use (one-time use per scroll)
- Zone file (98.zon) loads scroll 9808 in angel inventory and in room 9823

## Original Special Procedures

These triggers replace the following C special procedures:
- `SPECIAL(angels)` - Angel greeting and scroll giving
- `SPECIAL(evil_boss)` - Boss fight mechanics and charmed mob cleanup
- `SPECIAL(room_9823)` - Room entry message
- `SPECIAL(scroll_9808)` - Scroll usage mechanics
