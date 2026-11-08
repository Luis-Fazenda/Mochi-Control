package br.com.mochicar.control

import android.Manifest
import android.annotation.SuppressLint
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothGatt
import android.bluetooth.BluetoothGattCallback
import android.bluetooth.BluetoothGattCharacteristic
import android.bluetooth.BluetoothGattDescriptor
import android.bluetooth.BluetoothManager
import android.bluetooth.BluetoothProfile
import android.bluetooth.le.ScanCallback
import android.bluetooth.le.ScanFilter
import android.bluetooth.le.ScanSettings
import android.content.Context
import android.content.pm.PackageManager
import android.os.Build
import android.os.Handler
import android.os.Looper
import android.os.ParcelUuid
import androidx.core.content.ContextCompat
import java.util.UUID

object BleMochi {
    const val SERVICE_UUID = "8b6f0001-6f3a-4b9d-9a4e-2a1b7c8d0001"
    const val COMMAND_UUID = "8b6f0002-6f3a-4b9d-9a4e-2a1b7c8d0001"
    private const val PREFS = "mochi_ble"
    private const val KEY_ADDRESS = "device_address"

    @Volatile var connected: Boolean = false
        private set
    @Volatile var statusText: String = "Desconectado"
        private set
    @Volatile var latestStatus: String = ""
        private set

    private var gatt: BluetoothGatt? = null
    private var commandCharacteristic: BluetoothGattCharacteristic? = null
    private var scanCallback: ScanCallback? = null

    fun hasPermissions(context: Context): Boolean {
        return if (Build.VERSION.SDK_INT >= 31) {
            ContextCompat.checkSelfPermission(context, Manifest.permission.BLUETOOTH_SCAN) == PackageManager.PERMISSION_GRANTED &&
            ContextCompat.checkSelfPermission(context, Manifest.permission.BLUETOOTH_CONNECT) == PackageManager.PERMISSION_GRANTED
        } else {
            ContextCompat.checkSelfPermission(context, Manifest.permission.ACCESS_FINE_LOCATION) == PackageManager.PERMISSION_GRANTED
        }
    }

    @SuppressLint("MissingPermission")
    fun scanAndConnect(context: Context, onUpdate: (String) -> Unit) {
        if (!hasPermissions(context)) {
            onUpdate("Permita dispositivos próximos e tente novamente.")
            return
        }
        val manager = context.getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager
        val adapter = manager.adapter
        if (adapter == null || !adapter.isEnabled) {
            onUpdate("Ative o Bluetooth do celular.")
            return
        }

        statusText = "Procurando Mochi-Car..."
        onUpdate(statusText)

        val scanner = adapter.bluetoothLeScanner ?: run {
            onUpdate("Bluetooth LE indisponível.")
            return
        }
        val settings = ScanSettings.Builder()
            .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
            .build()

        // Busca sem filtro de UUID: alguns aparelhos não entregam o UUID
        // no pacote inicial do anúncio BLE, embora o serviço exista.
        scanCallback = object : ScanCallback() {
            override fun onScanResult(callbackType: Int, result: android.bluetooth.le.ScanResult) {
                val advertisedName = result.scanRecord?.deviceName ?: result.device.name
                val advertisedServices = result.scanRecord?.serviceUuids.orEmpty()
                val hasMochiService = advertisedServices.any {
                    it.uuid == UUID.fromString(SERVICE_UUID)
                }
                val isMochiName = advertisedName == "Mochi-Car"

                if (!isMochiName && !hasMochiService) return

                scanner.stopScan(this)
                scanCallback = null
                statusText = "Mochi encontrado; conectando..."
                onUpdate(statusText)
                connect(context.applicationContext, result.device, onUpdate)
            }

            override fun onScanFailed(errorCode: Int) {
                scanCallback = null
                statusText = "Falha na busca BLE: $errorCode"
                onUpdate(statusText)
            }
        }
        scanner.startScan(null, settings, scanCallback)

        // Encerra a busca após 20 segundos e mostra uma mensagem útil.
        Handler(Looper.getMainLooper()).postDelayed({
            val active = scanCallback
            if (active != null) {
                try { scanner.stopScan(active) } catch (_: Exception) {}
                scanCallback = null
                statusText = "Mochi não encontrado. Confira se o ESP32 está ligado e anuncia Mochi-Car."
                onUpdate(statusText)
            }
        }, 20_000L)
    }

    @SuppressLint("MissingPermission")
    private fun connect(context: Context, device: BluetoothDevice, onUpdate: (String) -> Unit) {
        gatt?.close()
        gatt = device.connectGatt(context, false, object : BluetoothGattCallback() {
            override fun onConnectionStateChange(g: BluetoothGatt, status: Int, newState: Int) {
                if (status == BluetoothGatt.GATT_SUCCESS && newState == BluetoothProfile.STATE_CONNECTED) {
                    gatt = g
                    connected = false
                    statusText = "Bluetooth conectado; verificando serviço do Mochi..."
                    context.getSharedPreferences(PREFS, Context.MODE_PRIVATE)
                        .edit().putString(KEY_ADDRESS, device.address).apply()
                    g.discoverServices()
                    onUpdate(statusText)
                } else {
                    connected = false
                    commandCharacteristic = null
                    statusText = "Conexão BLE perdida"
                    onUpdate(statusText)
                    g.close()
                }
            }

            override fun onCharacteristicChanged(
                g: BluetoothGatt,
                characteristic: BluetoothGattCharacteristic,
                value: ByteArray
            ) {
                if (characteristic.uuid == UUID.fromString(COMMAND_UUID)) {
                    latestStatus = value.toString(Charsets.UTF_8)
                }
            }

            @Suppress("DEPRECATION")
            override fun onCharacteristicChanged(
                g: BluetoothGatt,
                characteristic: BluetoothGattCharacteristic
            ) {
                if (characteristic.uuid == UUID.fromString(COMMAND_UUID)) {
                    latestStatus = characteristic.value?.toString(Charsets.UTF_8) ?: ""
                }
            }

            override fun onServicesDiscovered(g: BluetoothGatt, status: Int) {
                if (status != BluetoothGatt.GATT_SUCCESS) {
                    connected = false
                    commandCharacteristic = null
                    statusText = "Falha ao descobrir serviços BLE (código $status)."
                    onUpdate(statusText)
                    return
                }

                val service = g.getService(UUID.fromString(SERVICE_UUID))
                if (service == null) {
                    connected = false
                    commandCharacteristic = null
                    statusText = "Bluetooth conectado, mas o serviço Mochi não foi encontrado. Verifique o UUID/firmware."
                    onUpdate(statusText)
                    return
                }

                val ch = service.getCharacteristic(UUID.fromString(COMMAND_UUID))
                if (ch == null) {
                    connected = false
                    commandCharacteristic = null
                    statusText = "Serviço Mochi encontrado, mas a característica de comando está ausente."
                    onUpdate(statusText)
                    return
                }

                commandCharacteristic = ch
                connected = true
                statusText = "Conectado ao Mochi-Car!"
                onUpdate(statusText)

                val notificationSet = g.setCharacteristicNotification(ch, true)
                val descriptor = ch.getDescriptor(UUID.fromString("00002902-0000-1000-8000-00805f9b34fb"))
                if (descriptor != null) {
                    if (Build.VERSION.SDK_INT >= 33) {
                        val result = g.writeDescriptor(descriptor, BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE)
                        if (result != BluetoothGatt.GATT_SUCCESS) {
                            statusText = "Conectado; aviso de estado BLE não ativado ($result)."
                            onUpdate(statusText)
                        }
                    } else {
                        @Suppress("DEPRECATION")
                        descriptor.value = BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE
                        @Suppress("DEPRECATION")
                        val result = g.writeDescriptor(descriptor)
                        if (!result) {
                            statusText = "Conectado; aviso de estado BLE não ativado."
                            onUpdate(statusText)
                        }
                    }
                } else if (!notificationSet) {
                    statusText = "Conectado; notificações BLE indisponíveis."
                    onUpdate(statusText)
                }
            }
        }, BluetoothDevice.TRANSPORT_LE)
    }

    @SuppressLint("MissingPermission")
    fun send(command: String): Boolean {
        val g = gatt ?: return false
        val ch = commandCharacteristic ?: return false
        if (!connected) return false
        val bytes = command.toByteArray(Charsets.UTF_8)
        return if (Build.VERSION.SDK_INT >= 33) {
            g.writeCharacteristic(ch, bytes, BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT) == BluetoothGatt.GATT_SUCCESS
        } else {
            @Suppress("DEPRECATION")
            ch.writeType = BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT
            @Suppress("DEPRECATION")
            ch.value = bytes
            @Suppress("DEPRECATION")
            g.writeCharacteristic(ch)
        }
    }

    @SuppressLint("MissingPermission")
    fun reconnectSaved(context: Context, onUpdate: (String) -> Unit = {}) {
        if (!hasPermissions(context)) return
        val address = context.getSharedPreferences(PREFS, Context.MODE_PRIVATE)
            .getString(KEY_ADDRESS, null) ?: return
        try {
            val manager = context.getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager
            val device = manager.adapter.getRemoteDevice(address)
            connect(context.applicationContext, device, onUpdate)
        } catch (_: Exception) {
            statusText = "Não foi possível reconectar"
            onUpdate(statusText)
        }
    }
}
