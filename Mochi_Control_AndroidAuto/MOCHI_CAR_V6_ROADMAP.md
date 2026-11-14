# Mochi Car V6 — plano de desenvolvimento

## Objetivo
Evoluir o Mochi Car e o Mochi Control com personalidades mais inteligentes, novas animações, estatísticas de viagem, visual musical, personalização e comunicação BLE mais confiável, sem regressões na versão aprovada.

## Regra de compatibilidade (obrigatória)
A V6 parte do commit estável `663ce98439ce65d22497aa2f5f1cd471eb799759`.
Não remover, renomear ou redesenhar as 34 expressões existentes, nem substituir seus desenhos por aproximações. Preservar os seis modos (Normal, Fofo, Engraçado, Rabugento, Sono e Automático), as telas Home/Menu/Music/Animations/Trip, os botões físicos, os comandos BLE existentes e a continuidade do cronômetro Trip ao trocar de tela.
Expressão escolhida manualmente permanece fixa. Somente uma nova seleção manual ou ativação explícita da variação pode liberar a troca automática. Eventos de conexão, música, viagem ou notificações não podem sobrescrever uma expressão fixa.

## Inventário técnico confirmado
- ESP32 DevKit V1 e OLED SSD1306 128x64 (I2C SDA 21, SCL 22).
- Botão Menu GPIO18 e botão Animações GPIO19.
- BLE: nome `Mochi-Car`; serviço `8b6f0001-6f3a-4b9d-9a4e-2a1b7c8d0001`; característica `8b6f0002-6f3a-4b9d-9a4e-2a1b7c8d0001`.
- O status BLE já transmite modo, expressão, tempo Trip, estado Trip, visual Music e tela.
- O app já contém atalhos, seis personalidades, 34 expressões, Music e Trip.
- Android Auto usa a Car App Library e deve manter interações simples e compatíveis com as regras de atenção do motorista.

## Escopo funcional da V6

### 1. Personalidades inteligentes
Criar perfis comportamentais configuráveis para cada personalidade, com frequência e intensidade de microanimações, piscadas, olhares e reações. Não alterar o desenho base das expressões existentes. O modo Automático pode alternar entre perfis; os demais permanecem previsíveis.

### 2. Animações e eventos
Adicionar sequências curtas de inicialização, conexão, desconexão e confirmação de comando. Eventos são temporários e só podem ocorrer quando a expressão não estiver bloqueada por seleção manual; a expressão fixa deve ser restaurada após qualquer evento permitido.

### 3. Trip e estatísticas
Manter o cronômetro atual. Acrescentar registro local de viagens, duração, início/fim e estatísticas de sessão. Distância, velocidade média e localização só serão habilitadas com uma fonte GPS válida e permissão explícita no telefone; sem GPS, o Trip continua funcionando como cronômetro, sem inventar valores.

### 4. Music Visualizer
Manter os três visuais atuais (WAVE, PULSE e RANDOM). Acrescentar um modo reativo somente se houver uma fonte de áudio autorizada e tecnicamente compatível. Não solicitar captura de microfone como padrão; se não houver fonte segura, oferecer visualização animada sintética.

### 5. Personalização
Adicionar favoritos e atalhos sem substituir as categorias atuais; permitir ajuste de brilho/velocidade apenas se o hardware e a biblioteca suportarem; persistir preferências no telefone e sincronizar com o ESP32 quando conectado. Editor visual de expressões é opcional e não deve substituir a seleção das 34 expressões originais.

### 6. BLE e estado
Fortalecer reconexão, confirmação de escrita, leitura de estado e tratamento de desconexão. Manter UUIDs e comandos atuais compatíveis. Novos comandos devem ter prefixos/versionamento e não podem bloquear o loop de animação.

## Fases e critérios de aceite
1. **Base e proteção contra regressões:** inventário de expressões/comandos e testes estáticos; criar branch de desenvolvimento isolada.
2. **BLE e estado:** confirmação/recuperação de conexão, atualização do estado no app e comandos compatíveis.
3. **Experiência visual:** eventos e microanimações, com teste de expressão fixa.
4. **Trip:** histórico local e estatísticas; GPS opcional, permissões claras e sem depender de GPS para cronômetro.
5. **Music:** visuais novos mantendo os três existentes; áudio reativo somente com fonte suportada e autorização.
6. **Personalização:** favoritos, preferências e atalhos sem remover telas ou expressões.
7. **Validação e entrega:** compilar APK; revisar firmware para ESP32; executar testes de comandos, cronômetro, troca de telas, reconexão e todas as 34 expressões. Não declarar firmware validado sem compilação real.

## Testes obrigatórios antes de integrar
- Todos os comandos antigos ainda são reconhecidos.
- As 34 expressões continuam presentes e selecionáveis.
- Expressão manual permanece fixa durante ocioso, troca de tela e eventos.
- Variação só ocorre após ativação explícita.
- Trip não zera nem pausa ao navegar entre telas.
- BLE reconecta após perda de sinal e não envia comandos duplicados.
- O loop do ESP32 continua sem delays longos.
- O APK compila no GitHub Actions.
- Teste físico de BLE/OLED deve ser feito com o veículo estacionado; o display não deve obstruir a visibilidade do motorista.

## Estratégia de entrega
Desenvolver nesta branch `mochi-car-v6-dev`. A branch `main` permanece como base estável. Cada fase deve gerar commits pequenos e verificáveis. Só integrar à `main` depois dos critérios de aceite e da aprovação do usuário.
