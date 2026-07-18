"""
// Caminho: aethernus/engines/yggdrasil.py
Motor Central Yggdrasil - O Coração do Aethernus.
Responsável por orquestrar o tick de 3 segundos e gerenciar os motores secundários.
Implementa os Artigos 3, 4, 5 e 14 da Constituição.
"""

import asyncio
import time
import importlib
import logging
from typing import Dict, Optional
from aethernus.core.ecs import EntityManager, System
from aethernus.core.constants import SERVER_TICK, EngineState

# Configuração de Logging Profissional
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s [YGGDRASIL] [%(levelname)s] %(message)s'
)
logger = logging.getLogger("Yggdrasil")

class Yggdrasil:
    """
    O Motor Primordial que sustenta todos os outros sistemas.
    Garante a pulsação física e a modularidade do ecossistema.
    """
    def __init__(self):
        self.entity_manager = EntityManager()
        self.engines: Dict[str, System] = {}
        self.engine_states: Dict[str, EngineState] = {}
        self.is_running = False
        self._last_tick_time = 0.0
        
        logger.info("Yggdrasil inicializada. Aguardando ignição dos motores.")

    def register_engine(self, name: str, engine: System) -> None:
        """
        Registra um novo motor secundário no ecossistema.
        Artigo 5: Motores secundários são adicionados à Yggdrasil.
        """
        self.engines[name] = engine
        self.engine_states[name] = EngineState.ONLINE
        logger.info(f"Motor '{name}' acoplado com sucesso.")

    async def start(self) -> None:
        """Inicia o laço de eventos principal do servidor."""
        self.is_running = True
        logger.info(f"Iniciando pulsação do servidor (Tick: {SERVER_TICK}s)...")
        
        try:
            while self.is_running:
                start_time = time.perf_counter()
                
                # Executa a pulsação
                await self._pulse()
                
                # Cálculo de drift para manter a precisão do tick
                execution_time = time.perf_counter() - start_time
                sleep_time = max(0, SERVER_TICK - execution_time)
                
                if execution_time > SERVER_TICK:
                    logger.warning(f"LAG DETECTADO: Pulsação levou {execution_time:.2f}s (Limite: {SERVER_TICK}s)")
                
                await asyncio.sleep(sleep_time)
        except asyncio.CancelledError:
            logger.info("Sinal de encerramento recebido. Desligando Yggdrasil.")
        finally:
            self.is_running = False

    async def _pulse(self) -> None:
        """
        Coordena uma única pulsação de todos os motores ativos.
        Artigo 10: Execução assíncrona e não-bloqueante.
        """
        current_time = time.time()
        dt = current_time - self._last_tick_time if self._last_tick_time > 0 else SERVER_TICK
        self._last_tick_time = current_time

        # Criar tarefas para todos os motores ONLINE
        tasks = []
        for name, engine in self.engines.items():
            if self.engine_states[name] == EngineState.ONLINE:
                tasks.append(self._run_engine_update(name, engine, dt))
        
        if tasks:
            await asyncio.gather(*tasks)

    async def _run_engine_update(self, name: str, engine: System, dt: float) -> None:
        """Executa o update de um motor com tratamento de exceção robusto."""
        try:
            await engine.update(dt)
        except Exception as e:
            logger.error(f"FALHA CRÍTICA no motor '{name}': {str(e)}", exc_info=True)
            # Artigo 15: Se falhar, podemos considerar colocar em STASIS
            # self.engine_states[name] = EngineState.STASIS

    def reload_engine(self, name: str) -> bool:
        """
        Artigo 14: Hot-Swapping de código em tempo real.
        Recarrega a lógica de um motor sem derrubar o servidor.
        """
        if name not in self.engines:
            logger.error(f"Tentativa de recarregar motor inexistente: {name}")
            return False

        try:
            # Esta é uma implementação simplificada da interface Janus
            # Em um cenário real, precisaríamos recarregar o módulo e reinstanciar a classe
            engine_module = importlib.import_module(self.engines[name].__module__)
            importlib.reload(engine_module)
            logger.info(f"Motor '{name}' recarregado via interface Janus.")
            return True
        except Exception as e:
            logger.error(f"Falha ao recarregar motor '{name}': {str(e)}")
            return False

    def stop(self) -> None:
        """Interrompe a execução do servidor."""
        self.is_running = False
        logger.info("Comando de parada executado.")
