package br.com.mochicar.control

import android.content.Intent
import androidx.car.app.CarAppService
import androidx.car.app.CarContext
import androidx.car.app.Screen
import androidx.car.app.Session
import androidx.car.app.model.Action
import androidx.car.app.model.GridItem
import androidx.car.app.model.GridTemplate
import androidx.car.app.model.Header
import androidx.car.app.model.ItemList
import androidx.car.app.validation.HostValidator
import androidx.core.graphics.drawable.IconCompat

class MochiCarAppService : CarAppService() {
    override fun createHostValidator(): HostValidator =
        HostValidator.ALLOW_ALL_HOSTS_VALIDATOR

    override fun onCreateSession(): Session = MochiSession()
}

class MochiSession : Session() {
    override fun onCreateScreen(intent: Intent): Screen {
        BleMochi.reconnectSaved(carContext)
        return MochiHomeScreen(carContext)
    }
}

class MochiHomeScreen(carContext: CarContext) : Screen(carContext) {
    override fun onGetTemplate(): GridTemplate {
        val list = ItemList.Builder()
        list.addItem(item("Personalidades", "Escolher modo") {
            screenManager.push(MochiModeScreen(carContext))
        })
        list.addItem(item("Animações", "Próxima expressão") {
            send("SCENE:NEXT")
        })
        list.addItem(item("Música", "Trocar visual") {
            send("MUSIC:NEXT")
        })
        list.addItem(item("Trip", "Iniciar ou pausar") {
            send("TRIP:TOGGLE")
        })
        list.addItem(item("Zerar Trip", "Reiniciar cronômetro") {
            send("TRIP:RESET")
        })
        list.addItem(item("Tela do Mochi", "Voltar para início") {
            send("SCREEN:HOME")
        })

        return GridTemplate.Builder()
            .setHeader(
                Header.Builder()
                    .setTitle(if (BleMochi.connected) "Mochi conectado" else "Mochi desconectado")
                    .setStartHeaderAction(Action.APP_ICON)
                    .build()
            )
            .setSingleList(list.build())
            .build()
    }

    private fun item(title: String, subtitle: String, action: () -> Unit): GridItem {
        val icon = IconCompat.createWithResource(carContext, R.drawable.ic_mochi)
        return GridItem.Builder()
            .setTitle(title)
            .setImage(
                androidx.car.app.model.CarIcon.Builder(icon).build(),
                GridItem.IMAGE_TYPE_ICON
            )
            .setOnClickListener {
                action()
                invalidate()
            }
            .build()
    }

    private fun send(command: String) {
        if (!BleMochi.send(command)) {
            // A tela continua funcional; a conexão pode ser refeita pelo app no celular.
        }
    }
}

class MochiModeScreen(carContext: CarContext) : Screen(carContext) {
    override fun onGetTemplate(): GridTemplate {
        val list = ItemList.Builder()
        val modes = listOf(
            "Normal" to "MODE:NORMAL",
            "Fofo" to "MODE:CUTE",
            "Engraçado" to "MODE:FUNNY",
            "Rabugento" to "MODE:GRUMPY",
            "Sono" to "MODE:SLEEP",
            "Automático" to "MODE:AUTO"
        )
        modes.forEach { (name, command) ->
            val icon = IconCompat.createWithResource(carContext, R.drawable.ic_mochi)
            list.addItem(
                GridItem.Builder()
                    .setTitle(name)
                    .setImage(
                        androidx.car.app.model.CarIcon.Builder(icon).build(),
                        GridItem.IMAGE_TYPE_ICON
                    )
                    .setOnClickListener {
                        BleMochi.send(command)
                        screenManager.pop()
                    }
                    .build()
            )
        }
        return GridTemplate.Builder()
            .setHeader(
                Header.Builder()
                    .setTitle("Personalidade")
                    .setStartHeaderAction(Action.BACK)
                    .build()
            )
            .setSingleList(list.build())
            .build()
    }
}
