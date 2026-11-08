package br.com.mochicar.control

import android.Manifest
import android.app.Activity
import android.os.Build
import android.os.Bundle
import android.view.Gravity
import android.view.View
import android.widget.Button
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.TextView
import android.widget.Toast

class SetupActivity : Activity() {
    private lateinit var status: TextView
    private lateinit var connectButton: Button
    private lateinit var controls: LinearLayout
    private val permissionRequest = 77

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(20, 18, 20, 18)
        }
        val scroll = ScrollView(this)
        val content = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            gravity = Gravity.CENTER_HORIZONTAL
        }
        val title = TextView(this).apply {
            text = "MOCHI CONTROL"
            textSize = 28f
            gravity = Gravity.CENTER
            setPadding(0, 10, 0, 8)
        }
        val subtitle = TextView(this).apply {
            text = "Controle as funções do seu Mochi Car"
            textSize = 15f
            gravity = Gravity.CENTER
            setPadding(0, 0, 0, 14)
        }
        status = TextView(this).apply {
            text = BleMochi.statusText
            textSize = 16f
            gravity = Gravity.CENTER
            setPadding(8, 12, 8, 12)
        }
        connectButton = Button(this).apply {
            text = if (BleMochi.connected) "CONECTADO" else "CONECTAR AO MOCHI"
            setOnClickListener { requestPermissionsThenScan() }
        }
        controls = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            visibility = View.GONE
        }

        content.addView(title)
        content.addView(subtitle)
        content.addView(status)
        content.addView(connectButton)
        addSection(controls, "TELAS DO MOCHI")
        addButton(controls, "Menu", "SCREEN:MENU")
        addButton(controls, "Início", "SCREEN:HOME")
        addButton(controls, "Música", "SCREEN:MUSIC")
        addButton(controls, "Animações", "SCREEN:ANIMATIONS")
        addButton(controls, "Trip", "SCREEN:TRIP")

        addSection(controls, "PERSONALIDADES")
        addButton(controls, "Normal", "MODE:NORMAL")
        addButton(controls, "Fofo", "MODE:CUTE")
        addButton(controls, "Engraçado", "MODE:FUNNY")
        addButton(controls, "Rabugento", "MODE:GRUMPY")
        addButton(controls, "Sono", "MODE:SLEEP")
        addButton(controls, "Automático", "MODE:AUTO")

        addSection(controls, "ANIMAÇÕES")
        addButton(controls, "Próxima expressão", "SCENE:NEXT")
        addButton(controls, "Feliz", "SCENE:HAPPY")
        addButton(controls, "Surpreso", "SCENE:SURPRISED")
        addButton(controls, "Sonolento", "SCENE:SLEEPY")
        addButton(controls, "Rindo", "SCENE:LAUGHING")
        addButton(controls, "Curioso", "SCENE:CURIOUS")

        addSection(controls, "MÚSICA")
        addButton(controls, "Trocar visual da música", "MUSIC:NEXT")

        addSection(controls, "TRIP")
        addButton(controls, "Iniciar / Pausar Trip", "TRIP:TOGGLE")
        addButton(controls, "Zerar Trip", "TRIP:RESET")

        val note = TextView(this).apply {
            text = "Use os controles com o veículo estacionado ou por um passageiro."
            textSize = 13f
            gravity = Gravity.CENTER
            setPadding(8, 18, 8, 20)
        }
        controls.addView(note)
        content.addView(controls)
        scroll.addView(content)
        root.addView(scroll)
        setContentView(root)

        updateConnectionUi(BleMochi.statusText)
        if (BleMochi.connected) controls.visibility = View.VISIBLE
    }

    private fun addSection(parent: LinearLayout, name: String) {
        parent.addView(TextView(this).apply {
            text = name
            textSize = 18f
            gravity = Gravity.CENTER_VERTICAL
            setPadding(4, 20, 4, 6)
        })
    }

    private fun addButton(parent: LinearLayout, label: String, command: String) {
        parent.addView(Button(this).apply {
            text = label
            isAllCaps = false
            setOnClickListener {
                if (!BleMochi.connected) {
                    Toast.makeText(this@SetupActivity, "Conecte o Mochi primeiro.", Toast.LENGTH_SHORT).show()
                } else {
                    val sent = BleMochi.send(command)
                    Toast.makeText(
                        this@SetupActivity,
                        if (sent) "Comando enviado: $label" else "Não foi possível enviar. Tente reconectar.",
                        Toast.LENGTH_SHORT
                    ).show()
                }
            }
        })
    }

    private fun updateConnectionUi(message: String) {
        status.text = message
        val isConnected = BleMochi.connected
        controls.visibility = if (isConnected) View.VISIBLE else View.GONE
        connectButton.text = if (isConnected) "CONECTADO AO MOCHI" else "CONECTAR AO MOCHI"
    }

    private fun requestPermissionsThenScan() {
        if (BleMochi.connected) {
            updateConnectionUi(BleMochi.statusText)
            return
        }
        val permissions = if (Build.VERSION.SDK_INT >= 31) {
            arrayOf(Manifest.permission.BLUETOOTH_SCAN, Manifest.permission.BLUETOOTH_CONNECT)
        } else {
            arrayOf(Manifest.permission.ACCESS_FINE_LOCATION)
        }
        val missing = permissions.filter {
            checkSelfPermission(it) != android.content.pm.PackageManager.PERMISSION_GRANTED
        }
        if (missing.isNotEmpty()) {
            requestPermissions(missing.toTypedArray(), permissionRequest)
        } else {
            connect()
        }
    }

    override fun onRequestPermissionsResult(
        requestCode: Int, permissions: Array<out String>, grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode == permissionRequest && grantResults.isNotEmpty() &&
            grantResults.all { it == android.content.pm.PackageManager.PERMISSION_GRANTED }) {
            connect()
        } else if (requestCode == permissionRequest) {
            updateConnectionUi("Permissão de dispositivos próximos não concedida.")
        }
    }

    private fun connect() {
        BleMochi.scanAndConnect(this) { message ->
            runOnUiThread { updateConnectionUi(message) }
        }
    }

    override fun onResume() {
        super.onResume()
        if (::status.isInitialized) updateConnectionUi(BleMochi.statusText)
    }
}
