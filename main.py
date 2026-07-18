"""
// Caminho: main.py
Ponto de Entrada Principal do Aethernus.
Responsável por inicializar a Yggdrasil, acoplar os motores e abrir os portais.
"""

import asyncio
import logging
from aethernus.engines.yggdrasil import Yggdrasil
from aethernus.engines.hermes import Hermes

# Configuração de Log Global
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s [%(name)s] [%(levelname)s] %(message)s'
)
logger = logging.getLogger("Aethernus")

async def bootstrap():
    """
    Inicializa o ecossistema e inicia a pulsação do mundo.
    """
    logger.info("--- INICIANDO SISTEMA AETHERNUS ---")
    
    # 1. Instancia o Motor Central
    yggdrasil = Yggdrasil()
    
    # 2. Instancia e Registra o Motor Hermes (Rede)
    hermes = Hermes(yggdrasil.entity_manager)
    yggdrasil.register_engine("Hermes", hermes)
    
    # 3. Configura tarefas concorrentes
    # A Yggdrasil roda o loop de lógica, enquanto o Hermes ouve conexões
    try:
        # Iniciamos o servidor do Hermes em background
        server_task = asyncio.create_task(hermes.start_server(host='0.0.0.0', port=4000))
        
        # Iniciamos a pulsação da Yggdrasil (Bloqueante para este script)
        await yggdrasil.start()
        
    except Exception as e:
        logger.error(f"FALHA NA INICIALIZAÇÃO: {str(e)}", exc_info=True)
    finally:
        logger.info("--- SISTEMA AETHERNUS ENCERRADO ---")

if __name__ == "__main__":
    try:
        asyncio.run(bootstrap())
    except KeyboardInterrupt:
        pass
