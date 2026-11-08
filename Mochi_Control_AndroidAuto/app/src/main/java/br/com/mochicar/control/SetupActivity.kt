package br.com.mochicar.control

import android.Manifest
import android.app.Activity
import android.os.Build
import android.os.Bundle
import android.view.Gravity
import android.widget.Button
import android.widget.LinearLayout
import android.widget.TextView

class SetupActivity : Activity() {
    private lateinit var status: TextView

    private val permissionRequest = 77

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        val layout = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            gravity = Gravity.CENTER
            setPadding(28, 28, 28, 28)
        }
        val title = TextView(this).apply {
            text = "MOCHI CONTROL"
            textSize = 26f
            gravity = Gravity.CENTER
        }
        status = TextView(this).apply {
            text = "Conecte o ESP32 ao celular por Bluetooth."
            textSize = 16f
            gravity = Gravity.CENTER
            setPadding(0, 24, 0, 24)
        }
        val connect = Button(this).apply {
            text = "Conectar ao Mochi"
            setOnClickListener { requestPermissionsThenScan() }
        }
        layout.addView(title)
        layout.addView(status)
        layout.addView(connect)
        setContentView(layout)

        if (BleMochi.connected) status.text = BleMochi.statusText
    }

    private fun requestPermissionsThenScan() {
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
        } else {
            status.text = "Permissão de dispositivos próximos não concedida."
        }
    }

    private fun connect() {
        BleMochi.scanAndConnect(this) { message ->
            runOnUiThread { status.text = message }
        }
    }
}
