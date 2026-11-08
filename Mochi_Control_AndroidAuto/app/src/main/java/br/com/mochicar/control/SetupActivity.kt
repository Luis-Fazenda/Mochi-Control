package br.com.mochicar.control

import android.Manifest
import android.app.Activity
import android.graphics.Color
import android.graphics.Typeface
import android.graphics.drawable.GradientDrawable
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
    private lateinit var connectButton: TextView
    private lateinit var controls: LinearLayout
    private val permissionRequest = 77

    private val bg = Color.rgb(10, 14, 28)
    private val panel = Color.rgb(21, 28, 48)
    private val tile = Color.rgb(30, 39, 64)
    private val muted = Color.rgb(158, 171, 198)
    private val white = Color.rgb(241, 245, 255)
    private val cyan = Color.rgb(75, 210, 235)
    private val purple = Color.rgb(153, 119, 255)

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.statusBarColor = bg
        window.navigationBarColor = bg
        if (Build.VERSION.SDK_INT >= 23) {
            window.decorView.systemUiVisibility = 0
        }

        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setBackgroundColor(bg)
        }
        val scroll = ScrollView(this).apply { clipToPadding = false }
        val content = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(dp(18), dp(18), dp(18), dp(30))
        }

        val brand = TextView(this).apply {
            text = "MOCHI  /  CONTROL"
            textSize = 12f
            letterSpacing = 0.18f
            setTextColor(cyan)
            typeface = Typeface.DEFAULT_BOLD
        }
        val title = TextView(this).apply {
            text = "Seu Mochi,\nno seu controle."
            textSize = 30f
            setTextColor(white)
            typeface = Typeface.DEFAULT_BOLD
            setPadding(0, dp(7), 0, dp(4))
        }
        val subtitle = TextView(this).apply {
            text = "Personalize as expressões e funções do painel."
            textSize = 14f
            setTextColor(muted)
        }

        val connectionCard = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            background = rounded(panel, 20)
            setPadding(dp(16), dp(14), dp(16), dp(14))
        }
        status = TextView(this).apply {
            textSize = 14f
            setTextColor(if (BleMochi.connected) cyan else muted)
            typeface = Typeface.DEFAULT_BOLD
        }
        connectButton = TextView(this).apply {
            text = "CONECTAR AO MOCHI"
            textSize = 14f
            gravity = Gravity.CENTER
            typeface = Typeface.DEFAULT_BOLD
            setTextColor(bg)
            background = rounded(cyan, 12)
            setPadding(dp(16), dp(13), dp(16), dp(13))
            setOnClickListener { requestPermissionsThenScan() }
        }
        connectionCard.addView(status)
        connectionCard.addView(connectButton, LinearLayout.LayoutParams(-1, -2).apply { topMargin = dp(12) })

        controls = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            visibility = View.GONE
        }

        content.addView(brand)
        content.addView(title)
        content.addView(subtitle, LinearLayout.LayoutParams(-1, -2).apply { bottomMargin = dp(18) })
        content.addView(connectionCard)
        content.addView(controls)

        addSection("ATALHOS", "Acesse rapidamente as telas")
        addGrid(
            listOf(
                Tile("⌂", "Início", "SCREEN:HOME", cyan),
                Tile("☷", "Menu", "SCREEN:MENU", purple),
                Tile("♫", "Música", "SCREEN:MUSIC", purple),
                Tile("✦", "Animações", "SCREEN:ANIMATIONS", cyan),
                Tile("◷", "Trip", "SCREEN:TRIP", cyan)
            )
        )

        addSection("PERSONALIDADE", "Escolha o jeito do Mochi")
        addGrid(
            listOf(
                Tile("●", "Normal", "MODE:NORMAL", cyan),
                Tile("♡", "Fofo", "MODE:CUTE", purple),
                Tile("笑", "Engraçado", "MODE:FUNNY", purple),
                Tile("☹", "Rabugento", "MODE:GRUMPY", cyan),
                Tile("☾", "Sono", "MODE:SLEEP", purple),
                Tile("⟳", "Automático", "MODE:AUTO", cyan)
            )
        )

        addSection("EXPRESSÕES ORIGINAIS", "As 11 carinhas clássicas do Mochi")
        addGrid(
            listOf(
                Tile("↻", "Próxima", "SCENE:NEXT", cyan),
                Tile("○", "Normal", "SCENE:NORMAL", cyan),
                Tile("☺", "Feliz", "SCENE:HAPPY", purple),
                Tile("☾", "Sonolento", "SCENE:SLEEPY", purple),
                Tile("☹", "Rabugento", "SCENE:GRUMPY", cyan),
                Tile("⌕", "Curioso", "SCENE:CURIOUS", purple),
                Tile("★", "Empolgado", "SCENE:EXCITED", cyan),
                Tile("♡", "Tímido", "SCENE:SHY", purple),
                Tile("!", "Surpreso", "SCENE:SURPRISED", cyan),
                Tile("?", "Confuso", "SCENE:CONFUSED", purple),
                Tile("笑", "Rindo", "SCENE:LAUGHING", cyan),
                Tile("…", "Entediado", "SCENE:BORED", purple)
            )
        )

        addSection("REAÇÕES PERSONALIZADAS", "Toques rápidos para interagir com simpatia")
        addGrid(
            listOf(
                Tile("😏", "Debochado", "SCENE:DEBOCHADO", purple),
                Tile("😂", "Gargalhada", "SCENE:GARGALHADA", cyan),
                Tile("😉", "Piscadinha", "SCENE:PISCADINHA", purple),
                Tile("😎", "Confiante", "SCENE:CONFIANTE", cyan),
                Tile("🤨", "Duvidando", "SCENE:DUVIDANDO", purple),
                Tile("😮", "Impressionado", "SCENE:IMPRESSIONADO", cyan),
                Tile("😠", "Irritado", "SCENE:IRRITADO", purple),
                Tile("😊", "Simpático", "SCENE:SIMPATICO", cyan),
                Tile("👋", "Olá", "SCENE:OLA", purple),
                Tile("🙏", "Obrigado", "SCENE:OBRIGADO", cyan),
                Tile("😅", "Foi mal", "SCENE:FOI_MAL", purple),
                Tile("♥", "Carinho", "SCENE:CARINHO", cyan)
            )
        )

        addSection("NOVAS EXPRESSÕES", "Mais olhares e emoções inspirados no RobotEye")
        addGrid(
            listOf(
                Tile("!", "Alerta", "SCENE:ALERTA", cyan),
                Tile("◎", "Focado", "SCENE:FOCADO", purple),
                Tile("☹", "Triste", "SCENE:TRISTE", cyan),
                Tile("◉", "Assustado", "SCENE:ASSUSTADO", purple),
                Tile("…", "Desesperado", "SCENE:DESESPERADO", cyan),
                Tile("↯", "Desorientado", "SCENE:DESORIENTADO", purple),
                Tile("?", "Preocupado", "SCENE:PREOCUPADO", cyan),
                Tile("←", "Olhar esquerda", "SCENE:OLHAR_ESQUERDA", purple),
                Tile("→", "Olhar direita", "SCENE:OLHAR_DIREITA", cyan),
                Tile("↑", "Olhar cima", "SCENE:OLHAR_CIMA", purple),
                Tile("↓", "Olhar baixo", "SCENE:OLHAR_BAIXO", cyan)
            )
        )

        addSection("MÚSICA", "Controles do visual musical")
        addGrid(listOf(Tile("♫", "Trocar visual", "MUSIC:NEXT", purple)))

        addSection("TRIP", "Cronômetro da viagem")
        addGrid(
            listOf(
                Tile("Ⅱ", "Iniciar / Pausar", "TRIP:TOGGLE", cyan),
                Tile("↺", "Zerar cronômetro", "TRIP:RESET", purple)
            )
        )

        val footer = TextView(this).apply {
            text = "MOCHI CAR  •  CONTROLE BLUETOOTH\nUse os controles com o veículo estacionado ou por um passageiro."
            textSize = 11f
            gravity = Gravity.CENTER
            setTextColor(muted)
            setPadding(dp(8), dp(22), dp(8), dp(10))
        }
        controls.addView(footer)

        scroll.addView(content)
        root.addView(scroll)
        setContentView(root)
        updateConnectionUi(BleMochi.statusText)
    }

    private data class Tile(val icon: String, val label: String, val command: String, val accent: Int)

    private fun addSection(title: String, caption: String) {
        val wrap = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(0, dp(22), 0, dp(10))
        }
        wrap.addView(TextView(this).apply {
            text = title
            textSize = 12f
            letterSpacing = 0.14f
            setTextColor(cyan)
            typeface = Typeface.DEFAULT_BOLD
        })
        wrap.addView(TextView(this).apply {
            text = caption
            textSize = 13f
            setTextColor(muted)
            setPadding(0, dp(3), 0, 0)
        })
        controls.addView(wrap)
    }

    private fun addGrid(items: List<Tile>) {
        var index = 0
        while (index < items.size) {
            val row = LinearLayout(this).apply { orientation = LinearLayout.HORIZONTAL }
            addTile(row, items[index], true)
            if (index + 1 < items.size) addTile(row, items[index + 1], false)
            else row.addView(View(this), LinearLayout.LayoutParams(0, dp(94), 1f))
            controls.addView(row)
            index += 2
        }
    }

    private fun addTile(row: LinearLayout, item: Tile, first: Boolean) {
        val card = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            gravity = Gravity.CENTER_VERTICAL
            background = rounded(tile, 16)
            setPadding(dp(13), dp(12), dp(10), dp(12))
            isClickable = true
            isFocusable = true
            setOnClickListener { sendCommand(item) }
        }
        card.addView(TextView(this).apply {
            text = item.icon
            textSize = 23f
            setTextColor(item.accent)
            typeface = Typeface.DEFAULT_BOLD
        })
        card.addView(TextView(this).apply {
            text = item.label
            textSize = 14f
            setTextColor(white)
            typeface = Typeface.DEFAULT_BOLD
            setPadding(0, dp(5), 0, 0)
        })
        val params = LinearLayout.LayoutParams(0, dp(88), 1f)
        if (first) params.rightMargin = dp(6) else params.leftMargin = dp(6)
        params.bottomMargin = dp(10)
        row.addView(card, params)
    }

    private fun sendCommand(item: Tile) {
        if (!BleMochi.connected) {
            Toast.makeText(this, "Conecte o Mochi primeiro.", Toast.LENGTH_SHORT).show()
            updateConnectionUi(BleMochi.statusText)
            return
        }
        val sent = BleMochi.send(item.command)
        Toast.makeText(
            this,
            if (sent) "Enviado: ${item.label}" else "Falha ao enviar. Reconecte o Mochi.",
            Toast.LENGTH_SHORT
        ).show()
    }

    private fun rounded(color: Int, radius: Int): GradientDrawable =
        GradientDrawable().apply {
            setColor(color)
            cornerRadius = dp(radius).toFloat()
        }

    private fun dp(value: Int): Int = (value * resources.displayMetrics.density).toInt()

    private fun updateConnectionUi(message: String) {
        status.text = if (BleMochi.connected) "●  CONECTADO AO MOCHI-CAR" else "○  $message"
        status.setTextColor(if (BleMochi.connected) cyan else muted)
        controls.visibility = if (BleMochi.connected) View.VISIBLE else View.GONE
        connectButton.text = if (BleMochi.connected) "CONECTADO  ✓" else "CONECTAR AO MOCHI"
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
        if (missing.isNotEmpty()) requestPermissions(missing.toTypedArray(), permissionRequest)
        else connect()
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
