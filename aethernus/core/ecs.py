"""
// Caminho: aethernus/core/ecs.py
Motor de Entidade-Componente-Sistema (ECS) do Aethernus.
Implementa o Artigo 9º da Constituição: "Todas as coisas no jogo serão Entidades... 
características definidas exclusivamente pelos Componentes".
"""

import uuid
from typing import Dict, List, Type, TypeVar, Optional, Set, Iterable
from dataclasses import dataclass, field

T = TypeVar('T', bound='Component')

@dataclass
class Component:
    """Base para todos os componentes. Deve ser uma dataclass pura."""
    pass

class Entity:
    """
    Um contêiner leve identificado por um UUID único.
    Não possui lógica interna, apenas armazena componentes.
    """
    def __init__(self, entity_id: Optional[uuid.UUID] = None):
        self.id: uuid.UUID = entity_id or uuid.uuid4()
        self._components: Dict[Type[Component], Component] = {}
        self.flags: Set[Enum] = set()  # Artigo 8: Sets para verificação rápida

    def add_component(self, component: Component) -> None:
        """Anexa um componente à entidade."""
        self._components[type(component)] = component

    def remove_component(self, component_type: Type[Component]) -> None:
        """Remove um componente da entidade."""
        if component_type in self._components:
            del self._components[component_type]

    def get_component(self, component_type: Type[T]) -> Optional[T]:
        """Recupera um componente específico pelo tipo."""
        return self._components.get(component_type)

    def has_component(self, component_type: Type[Component]) -> bool:
        """Verifica se a entidade possui um tipo de componente."""
        return component_type in self._components

    def __repr__(self) -> str:
        return f"Entity(id={self.id}, components={list(self._components.keys())})"

class EntityManager:
    """
    Orquestrador central do ciclo de vida das entidades.
    Permite consultas eficientes baseadas em assinaturas de componentes.
    """
    def __init__(self):
        self._entities: Dict[uuid.UUID, Entity] = {}
        # Cache de índices para busca rápida (opcional, implementado conforme escala)
        self._component_map: Dict[Type[Component], Set[uuid.UUID]] = {}

    def create_entity(self, entity_id: Optional[uuid.UUID] = None) -> Entity:
        """Cria e registra uma nova entidade."""
        entity = Entity(entity_id)
        self._entities[entity.id] = entity
        return entity

    def destroy_entity(self, entity_id: uuid.UUID) -> None:
        """Remove uma entidade e todos os seus componentes do sistema."""
        if entity_id in self._entities:
            entity = self._entities[entity_id]
            # Limpar índices
            for comp_type in entity._components.keys():
                self._component_map.get(comp_type, set()).discard(entity_id)
            del self._entities[entity_id]

    def register_component(self, entity_id: uuid.UUID, component: Component) -> None:
        """Registra um componente em uma entidade e atualiza índices."""
        if entity_id in self._entities:
            entity = self._entities[entity_id]
            entity.add_component(component)
            
            comp_type = type(component)
            if comp_type not in self._component_map:
                self._component_map[comp_type] = set()
            self._component_map[comp_type].add(entity_id)

    def get_entity(self, entity_id: uuid.UUID) -> Optional[Entity]:
        """Busca uma entidade pelo ID."""
        return self._entities.get(entity_id)

    def get_entities_with(self, *component_types: Type[Component]) -> Iterable[Entity]:
        """
        Retorna todas as entidades que possuem todos os componentes especificados.
        Implementação otimizada via interseção de Sets.
        """
        if not component_types:
            return self._entities.values()

        sets = [self._component_map.get(ct, set()) for ct in component_types]
        if not sets:
            return []
            
        target_ids = set.intersection(*sets)
        return (self._entities[eid] for eid in target_ids if eid in self._entities)

class System:
    """
    Base para todos os motores (Engines).
    Contém a lógica que processa grupos de entidades em cada tick.
    """
    def __init__(self, entity_manager: EntityManager):
        self.entity_manager = entity_manager
        self.is_active = True

    async def update(self, dt: float) -> None:
        """
        Método a ser sobrescrito pelos motores específicos.
        Artigo 10: Implementação assíncrona obrigatória.
        """
        raise NotImplementedError("Motores devem implementar o método update.")
