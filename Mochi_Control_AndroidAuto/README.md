# Mochi Control — Android Auto + ESP32 BLE

Este projeto usa Bluetooth Low Energy (BLE) entre o celular e o ESP32. O celular mantém o Wi-Fi conectado ao Android Auto sem fio.

Este projeto contém:
- `Mochi_Car_V5_BLE.ino`: firmware do ESP32 derivado do Mochi Car V5.
- `Mochi_Control_AndroidAuto/`: aplicativo Android com categoria IoT do Android for Cars App Library.

## Arquitetura

Android Auto (projetado pelo celular) -> app Mochi Control no telefone -> BLE -> ESP32.

O firmware BLE não cria uma rede Wi-Fi. Isso evita pedir que o telefone abandone a conexão Wi-Fi usada pelo Android Auto sem fio.

## 1. Gravar o ESP32

1. Abra `Mochi_Car_V5_BLE.ino` no Arduino IDE.
2. Instale as bibliotecas que o V5 já usava: Adafruit GFX, Adafruit SSD1306 e FluxGarage RoboEyes.
3. Use uma versão do pacote ESP32 Arduino que inclua a biblioteca BLE (`BLEDevice.h`).
4. Selecione sua placa ESP32 Dev Module e grave.
5. No Monitor Serial (115200), deve aparecer `BLE Mochi iniciado. Nome: Mochi-Car`.

O BLE usa uma característica compartilhada para receber comandos e notificar o estado do Mochi. O aplicativo deve ser conectado ao ESP32 pelo botão **Conectar ao Mochi** no celular antes de abrir o Android Auto.

O BLE usa:
- Nome: `Mochi-Car`
- Serviço: `8b6f0001-6f3a-4b9d-9a4e-2a1b7c8d0001`
- Característica de comando/status: `8b6f0002-6f3a-4b9d-9a4e-2a1b7c8d0001`

## 2. Abrir o app no Android Studio

1. Instale o Android Studio estável.
2. Extraia o ZIP e abra a pasta `Mochi_Control_AndroidAuto`.
3. Aguarde o Gradle sincronizar (é necessária internet para baixar dependências).
4. Conecte o celular Android por USB e execute `app`.
5. No app do celular, toque em **Conectar ao Mochi** e permita **Dispositivos próximos**.
6. Aguarde aparecer `Conectado ao Mochi-Car`.

## 3. Testar Android Auto

1. Primeiro confirme que o app conecta ao ESP32 pelo Bluetooth no celular.
2. Conecte o Android Auto sem fio à multimídia.
3. Procure `Mochi Control` na lista de aplicativos do Android Auto.
4. Se não aparecer durante o desenvolvimento, ative as opções de desenvolvedor do Android Auto e a opção de fontes desconhecidas, quando disponível, e confirme que o app foi instalado no celular.
5. A distribuição pública exige cumprir as diretrizes de qualidade e o processo de revisão do Google para apps de carro. A categoria IoT é suportada, mas isso não significa aprovação automática.

## Comandos BLE

- `MODE:NORMAL`, `MODE:CUTE`, `MODE:FUNNY`, `MODE:GRUMPY`, `MODE:SLEEP`, `MODE:AUTO`
- `SCENE:NEXT`, `SCENE:HAPPY`, `SCENE:SURPRISED`, `SCENE:SLEEPY`, `SCENE:LAUGHING`, `SCENE:CURIOUS`
- `MUSIC:NEXT`
- `TRIP:TOGGLE`, `TRIP:RESET`
- `SCREEN:HOME`, `SCREEN:MENU`, `SCREEN:MUSIC`, `SCREEN:ANIMATIONS`, `SCREEN:TRIP`

## Observações

- O Trip continua contando quando você muda de tela; só para ou zera por comando.
- A tela Android Auto usa modelos oficiais e não mostra animações ou conteúdo visual complexo durante a condução.
- Para evitar distrações, configure e teste o sistema com o veículo estacionado. Durante a viagem, deixe a interação para um passageiro.
- Este pacote é código-fonte. Não inclui APK já compilado e não foi testado em uma multimídia física.


## Correção desta revisão

- Corrigida a leitura BLE no firmware: `getValue()` é armazenado em `String`, compatível com a versão da biblioteca que gerou o erro de conversão para `std::string`.
- O comando recebido é copiado para o buffer de forma limitada, evitando ultrapassar o tamanho do buffer.
- Comentários e documentação alinhados com a arquitetura BLE (não Wi-Fi).

## Limites de teste

Este pacote contém código-fonte, não um APK compilado. A compilação do aplicativo depende do Android SDK/Gradle instalado no computador. A exibição do app no Android Auto durante desenvolvimento depende das configurações do Android Auto e da compatibilidade do host; a categoria IoT é suportada pela Car App Library, mas não representa aprovação automática para distribuição.


## Ajuste de compatibilidade do Android Gradle Plugin

O projeto usa Android Gradle Plugin 8.9.2. O `compileSdk` foi alinhado para API 35, evitando solicitar uma plataforma mais nova do que a faixa oficialmente suportada por essa versão do plugin. O `targetSdk` permanece em 35.

## Verificação realizada nesta revisão

- Conferência estática dos UUIDs BLE entre o firmware e o aplicativo.
- Conferência dos comandos emitidos pelo aplicativo com os comandos tratados pelo firmware.
- Correção da conversão `String`/`std::string` no callback BLE.
- Conferência da estrutura do ZIP e dos arquivos Gradle/Manifest.

A compilação real do APK e do firmware ainda precisa ser executada no computador com Android Studio, Android SDK, Gradle e Arduino IDE instalados.


## Gerar o APK automaticamente pelo GitHub Actions

O projeto inclui `.github/workflows/build-apk.yml`. Para gerar o APK na nuvem:

1. Crie um repositório no GitHub (pode ser privado) chamado `Mochi-Control`.
2. Extraia este ZIP no computador.
3. Envie **o conteúdo da pasta `Mochi_Control_AndroidAuto`** para a raiz do repositório. É importante incluir a pasta oculta `.github/workflows`.
4. No GitHub, abra a aba **Actions** e selecione **Build Mochi Control APK**.
5. Selecione **Run workflow** e confirme.
6. Aguarde o job ficar verde. Abra a execução concluída, vá até **Artifacts** e baixe `Mochi-Control-debug-apk`.
7. Extraia o ZIP do artifact. Dentro estará `app-debug.apk`.

O workflow também inicia automaticamente quando houver um push nas branches `main` ou `master`.

### Observações

- O artifact é um APK de depuração (`debug`), adequado para instalação e testes; não é uma versão assinada para publicação na Play Store.
- O GitHub Actions compila o código; ele não testa conexão BLE física nem garante que a multimídia aceite o app no Android Auto.
- Se a compilação falhar, abra a execução em Actions, copie o primeiro bloco `FAILURE: Build failed with an exception` e envie aqui para correção.
- Não coloque chaves privadas, senhas ou arquivos de assinatura no repositório.


## GitHub Actions APK

O workflow `.github/workflows/build-apk.yml` compila o APK de depuração automaticamente quando há alterações nesta pasta ou quando executado manualmente pela aba Actions. O arquivo gerado fica disponível no artifact `Mochi-Control-debug-apk`.


CI: configuração do Android SDK atualizada para instalar apenas platform-tools, evitando o pacote legado `tools` que foi removido do repositório do SDK.
