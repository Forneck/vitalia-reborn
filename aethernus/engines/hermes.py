"""
// Caminho: aethernus/engines/hermes.py
Motor Hermes - Camada de Conectividade e Rede.
Implementa a arquitetura assíncrona, gestão de soquetes e buffer de comandos.
Conforme Artigos 10, 11, 12 e 13 da Constituição.
"""

import asyncio
import logging
from typing import Dict, Optional, Set
from aethernus.core.ecs import System, EntityManager, Component, Entity
from aethernus.core.constants import EntityFlag

logger = logging.getLogger("Hermes")

class NetworkConnectionComponent(Component):
    """Armazena os fluxos de leitura/escrita de uma entidade conectada."""
    def __init__(self, reader: asyncio.StreamReader, writer: asyncio.StreamWriter):
        self.reader = reader
        self.writer = writer
        self.address = writer.get_extra_info('peername')

class CommandBufferComponent(Component):
    """
    Fila de comandos da entidade.
    Artigo 12: Separação de comandos de interação/locomoção de ações táticas.
    """
    def __init__(self):
        self.high_priority_queue: asyncio.Queue = asyncio.Queue()  # Movimento/Speedwalk
        self.tactical_queue: asyncio.Queue = asyncio.Queue()       # Combate/Magia (1 por tick)

class Hermes(System):
    """
    O Mensageiro Divino. Gerencia a entrada e saída de dados sem bloquear a Yggdrasil.
    """
    def __init__(self, entity_manager: EntityManager):
        super().__init__(entity_manager)
        self.clients: Dict[Entity, NetworkConnectionComponent] = {}
        self.server: Optional[asyncio.AbstractServer] = None

    async def start_server(self, host: str = '0.0.0.0', port: int = 4000) -> None:
        """Inicia o servidor TCP assíncrono."""
        self.server = await asyncio.start_server(self.handle_client, host, port)
        addr = self.server.sockets[0].getsockname()
        logger.info(f"Hermes ouvindo em {addr}. Portal aberto.")
        
        async with self.server:
            await self.server.serve_forever()

    async def handle_client(self, reader: asyncio.StreamReader, writer: asyncio.StreamWriter) -> None:
        """
        Artigo 11: Ciclo de vida da conectividade.
        Pipeline: Raw Socket -> Auth (Moira) -> ECS Entity -> Inject.
        """
        addr = writer.get_extra_info('peername')
        logger.info(f"Nova conexão bruta de {addr}")

        try:
            # Placeholder para o Motor Moira (Autenticação)
            # Por enquanto, criamos uma entidade temporária para teste
            entity = self.entity_manager.create_entity()
            
            # Anexa componentes de rede e comando
            conn = NetworkConnectionComponent(reader, writer)
            buffer = CommandBufferComponent()
            
            self.entity_manager.register_component(entity.id, conn)
            self.entity_manager.register_component(entity.id, buffer)
            
            self.clients[entity] = conn
            
            writer.write(b"Bem-vindo ao Aethernus.\nDigite seu nome: ")
            await writer.drain()

            # Laço de leitura contínua (Não-bloqueante)
            while True:
                data = await reader.read(1024)
                if not data:
                    break
                
                command = data.decode().strip()
                if command:
                    # Artigo 12: Injeta no Command Buffer
                    # Aqui aplicaríamos a lógica de triagem (Movimento vs Tático)
                    await buffer.high_priority_queue.put(command)
                    
        except ConnectionResetError:
            logger.warning(f"Conexão perdida abruptamente com {addr}")
        finally:
            # Artigo 13: Estado Linkdead
            logger.info(f"Conexão com {addr} encerrada. Marcando como LINKDEAD.")
            if 'entity' in locals():
                entity.flags.add(EntityFlag.LINKDEAD)
                # O motor Chronos cuidará da remoção após o timeout
            writer.close()
            await writer.wait_closed()

    async def update(self, dt: float) -> None:
        """
        Processa o buffer de comandos de todas as entidades conectadas.
        Executado a cada pulsação da Yggdrasil.
        """
        # Busca entidades com CommandBufferComponent
        for entity in self.entity_manager.get_entities_with(CommandBufferComponent):
            buffer = entity.get_component(CommandBufferComponent)
            
            # 1. Processa Movimentos (Speedwalking - Rajada)
            while not buffer.high_priority_queue.empty():
                cmd = await buffer.high_priority_queue.get()
                logger.debug(f"Processando comando de movimento: {cmd}")
                # Encaminhar para motor de locomoção...

            # 2. Processa Ações Táticas (Limite de 1 por tick - Art. 12)
            if not buffer.tactical_queue.empty():
                cmd = await buffer.tactical_queue.get()
                logger.debug(f"Processando comando tático: {cmd}")
                # Encaminhar para Ares/Hécate...
