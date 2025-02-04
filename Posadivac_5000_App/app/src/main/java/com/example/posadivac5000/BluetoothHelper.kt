package com.example.posadivac5000

import android.Manifest
import android.bluetooth.*
import android.bluetooth.le.*
import android.content.Context
import android.content.pm.PackageManager
import android.os.Build
import android.os.Handler
import android.os.Looper
import androidx.core.app.ActivityCompat
import java.util.UUID

class BluetoothHelper(private val context: Context, private val listener: BluetoothListener) {

    private val serviceUUID = UUID.fromString("676fa518-e4cb-4afa-aae4-f211fe532d48")
    private val characteristicUUID = UUID.fromString("a08ae7a0-11e8-483d-940c-a23d81245500")

    private val bluetoothManager: BluetoothManager = context.getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager
    private val bluetoothAdapter: BluetoothAdapter? = bluetoothManager.adapter
    private var bluetoothGatt: BluetoothGatt? = null
    private val bluetoothLeScanner: BluetoothLeScanner? = bluetoothAdapter?.bluetoothLeScanner

    fun initBluetooth(): Boolean {
        return bluetoothAdapter != null
    }

    fun isBluetoothEnabled(): Boolean {
        return bluetoothAdapter?.isEnabled ?: false
    }

    fun startScan() {
        if (bluetoothAdapter == null || bluetoothLeScanner == null) {
            listener.onError("Bluetooth nije podržan na uređaju.")
            return
        }

        if (bluetoothGatt != null) {
            listener.onConnected()
            return
        }

        if (!isBluetoothEnabled()) {
            listener.onError("Uključi Bluetooth!")
            return
        }

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            if (ActivityCompat.checkSelfPermission(context, Manifest.permission.BLUETOOTH_SCAN)
                != PackageManager.PERMISSION_GRANTED ||
                ActivityCompat.checkSelfPermission(context, Manifest.permission.BLUETOOTH_CONNECT)
                != PackageManager.PERMISSION_GRANTED
            ) {
                listener.onError("Nedostaju potrebne dozvole za BLE skeniranje.")
                return
            }
        }

        val scanSettings = ScanSettings.Builder()
            .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
            .build()

        bluetoothLeScanner.startScan(null, scanSettings, scanCallback)

        Handler(Looper.getMainLooper()).postDelayed({
            bluetoothLeScanner.stopScan(scanCallback)
            if (bluetoothGatt == null) {
                listener.onScanCompleted(false)
            }
        }, 10000)
    }

    private val scanCallback = object : ScanCallback() {
        override fun onScanResult(callbackType: Int, result: ScanResult) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
                if (ActivityCompat.checkSelfPermission(context, Manifest.permission.BLUETOOTH_SCAN)
                    != PackageManager.PERMISSION_GRANTED ||
                    ActivityCompat.checkSelfPermission(context, Manifest.permission.BLUETOOTH_CONNECT)
                    != PackageManager.PERMISSION_GRANTED
                ) {
                    listener.onError("Nedostaju potrebne dozvole za BLE.")
                    return
                }
            }
            val foundDeviceName = result.device.name ?: "Nepoznato"
            if (foundDeviceName == "ESP32-Posadivac5000") {

                bluetoothLeScanner?.stopScan(this)
                listener.onDeviceFound(foundDeviceName)
                connectToDevice(result.device)
            }
        }

        override fun onScanFailed(errorCode: Int) {
            listener.onError("Skeniranje nije uspjelo: $errorCode")
        }
    }

    private fun connectToDevice(device: BluetoothDevice) {
        if (ActivityCompat.checkSelfPermission(context, Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
            listener.onError("Dozvola za povezivanje nije odobrena.")
            return
        }
        bluetoothGatt = device.connectGatt(context, false, gattCallback)
    }

    private val gattCallback = object : BluetoothGattCallback() {
        override fun onConnectionStateChange(gatt: BluetoothGatt?, status: Int, newState: Int) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
                if (ActivityCompat.checkSelfPermission(context, Manifest.permission.BLUETOOTH_SCAN)
                    != PackageManager.PERMISSION_GRANTED ||
                    ActivityCompat.checkSelfPermission(context, Manifest.permission.BLUETOOTH_CONNECT)
                    != PackageManager.PERMISSION_GRANTED
                ) {
                    listener.onError("Nedostaju potrebne dozvole za BLE.")
                    return
                }
            }

            if (newState == BluetoothGatt.STATE_CONNECTED) {
                listener.onConnected()
                gatt?.requestMtu(512)
                gatt?.discoverServices()
            } else if (newState == BluetoothGatt.STATE_DISCONNECTED) {
                listener.onDisconnected()
                bluetoothGatt?.close()
                bluetoothGatt = null
            }
        }

        override fun onServicesDiscovered(gatt: BluetoothGatt?, status: Int) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
                if (ActivityCompat.checkSelfPermission(context, Manifest.permission.BLUETOOTH_SCAN)
                    != PackageManager.PERMISSION_GRANTED ||
                    ActivityCompat.checkSelfPermission(context, Manifest.permission.BLUETOOTH_CONNECT)
                    != PackageManager.PERMISSION_GRANTED
                ) {
                    listener.onError("Nedostaju potrebne dozvole za BLE.")
                    return
                }
            }

            if (status == BluetoothGatt.GATT_SUCCESS) {
                val characteristic = gatt?.getService(serviceUUID)?.getCharacteristic(characteristicUUID)
                if (characteristic != null) {
                    gatt.setCharacteristicNotification(characteristic, true)
                    val descriptor = characteristic.getDescriptor(UUID.fromString("00002902-0000-1000-8000-00805f9b34fb"))
                    descriptor.value = BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE
                    gatt.writeDescriptor(descriptor)
                }
            }
        }

        override fun onCharacteristicChanged(gatt: BluetoothGatt?, characteristic: BluetoothGattCharacteristic?) {
            characteristic?.value?.let { data ->
                val receivedData = String(data)
                listener.onDataReceived(receivedData)
            }
        }


    }

    fun disconnect() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            if (ActivityCompat.checkSelfPermission(context, Manifest.permission.BLUETOOTH_SCAN)
                != PackageManager.PERMISSION_GRANTED ||
                ActivityCompat.checkSelfPermission(context, Manifest.permission.BLUETOOTH_CONNECT)
                != PackageManager.PERMISSION_GRANTED
            ) {
                listener.onError("Nedostaju potrebne dozvole za BLE.")
                return
            }
        }
        bluetoothGatt?.disconnect()
    }

    interface BluetoothListener {
        fun onDeviceFound(deviceName: String)
        fun onConnected()
        fun onDisconnected()
        fun onDataReceived(data: String)
        fun onError(message: String)
        fun onScanCompleted(success: Boolean)
    }
}
