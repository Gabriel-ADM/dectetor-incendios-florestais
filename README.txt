================================================================================
SISTEMAS OPERACIONAIS - 2025/2
PROFESSOR: João Victor de A. Oliveira
================================================================================

PROJETO: Simulador de um Sistema Detector de Incêndios Florestais

AUTORES:
1. Marlon Ferreira de Sá
2. Gabriel de Oliveira Pinho

================================================================================
DESCRIÇÃO DO PROJETO
================================================================================
Este software simula uma rede de sensores para detecção de incêndios em uma 
floresta, representada por uma matriz 30x30. O sistema utiliza programação 
concorrente para simular o comportamento de nós sensores que monitoram o 
ambiente, comunicam-se entre si para propagar alertas e enviam 
informações para uma Central de Controle.

================================================================================
REQUISITOS DE SISTEMA
================================================================================
- Sistema Operacional: Linux .
- Compilador: GCC.

================================================================================
INSTRUÇÕES DE COMPILAÇÃO
================================================================================
Para compilar o projeto, abra o terminal no diretório onde o arquivo fonte 
está salvo e execute o seguinte comando:

    gcc main.c -o main -pthread

================================================================================
INSTRUÇÕES DE EXECUÇÃO
================================================================================
Após a compilação bem-sucedida, execute o programa com o comando:

    ./main

================================================================================
COMO UTILIZAR
================================================================================
1. Ao iniciar, o programa exibirá a grade 30x30 representando a floresta.
   - Caracteres verdes '-' representam áreas seguras.
   - Caracteres 'T' representam os sensores.
   - Caracteres vermelhos '@' representam focos de incêndio.

2. O sistema gera incêndios aleatórios a cada 5 segundos.

3. O monitoramento é automático. Quando um incêndio é detectado e a informação 
   chega à borda da matriz, o evento é registrado no arquivo "incendios.log" 
   criado no mesmo diretório.

4. Para encerrar a simulação, utilize o comando de interrupção do terminal:
   Ctrl + C
