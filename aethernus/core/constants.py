"""
// Caminho: aethernus/core/constants.py
Códice de Constantes e Flags Primordiais do Aethernus.
Governa o estado global, pulsação do servidor e identificadores de atributos.
"""

from enum import Enum, auto
from typing import Final

# --- Artigo 3: O Relógio do Servidor ---
SERVER_TICK: Final[float] = 3.0  # Segundos

# --- Artigo 15: Toggles de Estado e Estase ---
class EngineState(Enum):
    """Estados operacionais dos motores do sistema."""
    ONLINE = auto()
    DEBUG = auto()
    STASIS = auto()

# --- Artigo 8 e 9: Padronização de Flags de Entidade ---
class EntityFlag(Enum):
    """
    Flags de estado para entidades ECS.
    Utilizadas em Sets para verificação rápida (O(1)).
    """
    # Estados de Conectividade (Art. 13)
    LINKDEAD = auto()
    
    # Estados Biológicos e Combate (Art. 65, 46)
    IS_CORPSE = auto()
    HUNTER = auto()
    
    # Estados de Visibilidade e Acesso
    GOD_ONLY = auto()
    IMMORTAL = auto()

# --- Artigo 49: A Matriz de Atributos Primordiais ---
class AttributeType(Enum):
    """Eixos fundamentais da alma regidos pelos motores divinos."""
    VIGOR = "Vigor de Ares"
    ASTUCIA = "Astúcia de Hermes"
    MENTE = "Mente de Hécate"
    SINTONIA = "Sintonia de Kairós"
    ESSENCIA = "Essência de Phanes"

# --- Artigo 22: Identificação Hierárquica (VNUM) ---
# Estrutura: RMMCCRRZZNNNN
VNUM_PATTERN: Final[str] = r"^\d{13}$"

# --- Artigo 16: A Passagem do Tempo ---
REAL_MINUTES_PER_GAME_DAY: Final[int] = 48
TICKS_PER_GAME_DAY: Final[int] = int((REAL_MINUTES_PER_GAME_DAY * 60) / SERVER_TICK)
