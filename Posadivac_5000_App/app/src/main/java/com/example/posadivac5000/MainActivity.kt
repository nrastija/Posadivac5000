package com.example.posadivac5000

import android.Manifest
import android.bluetooth.*
import android.bluetooth.le.*
import android.content.Context
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.util.Log
import android.widget.Button
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat

class MainActivity : AppCompatActivity() {

    private val deviceName = "ESP32-App"
    private lateinit var bluetoothAdapter: BluetoothAdapter
    private var bluetoothGatt: BluetoothGatt? = null
    private lateinit var bluetoothLeScanner: BluetoothLeScanner
    private lateinit var statusTextView: TextView
    private lateinit var scanButton: Button

    private val SERVICE_UUID = "676fa518-e4cb-4afa-aae4-f211fe532d48"
    private val CHARACTERISTIC_UUID = "a08ae7a0-11e8-483d-940c-a23d81245500"

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        // UI elementi
        statusTextView = findViewById(R.id.statusTextView)
        scanButton = findViewById(R.id.btn_scan)

        Log.d("BLE", "Pokretanje aplikacije...")

        // Inicijalizacija Bluetooth-a
        initBluetooth()

        scanButton.setOnClickListener {
            Log.d("BLE", "Gumb za skeniranje pritisnut")
            checkPermissionsAndStartScan()
        }
    }

    // Funkcija za inicijalizaciju Bluetooth adaptera
    private fun initBluetooth() {
        val bluetoothManager = getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager
        bluetoothAdapter = bluetoothManager.adapter ?: run {
            Log.e("BLE", "Bluetooth nije podržan na ovom uređaju")
            return
        }
        bluetoothLeScanner = bluetoothAdapter.bluetoothLeScanner
        Log.d("BLE", "Bluetooth adapter uspješno inicijaliziran")
    }

    // Provjera dozvola prije skeniranja
    private fun checkPermissionsAndStartScan() {
        Log.d("BLE", "Provjera dozvola za BLE skeniranje...")
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            if (ContextCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_SCAN)
                != PackageManager.PERMISSION_GRANTED ||
                ContextCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_CONNECT)
                != PackageManager.PERMISSION_GRANTED
            ) {
                Log.d("BLE", "Dozvole nisu odobrene, tražim dozvole...")
                ActivityCompat.requestPermissions(
                    this,
                    arrayOf(
                        Manifest.permission.BLUETOOTH_SCAN,
                        Manifest.permission.BLUETOOTH_CONNECT
                    ),
                    101
                )
            } else {
                startBLEScan()
            }
        } else {
            startBLEScan()
        }
    }

    // Funkcija za pokretanje BLE skeniranja
    private fun startBLEScan() {
        Log.d("BLE", "Počeo pokušaj skeniranja svih BLE uređaja...")

        if (!bluetoothAdapter.isEnabled) {
            statusTextView.text = "Uključi Bluetooth!"
            Log.e("BLE", "Bluetooth nije uključen")
            return
        }

        // Provjera dozvola prije pokretanja skeniranja
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            if (ContextCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_SCAN)
                != PackageManager.PERMISSION_GRANTED ||
                ContextCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_CONNECT)
                != PackageManager.PERMISSION_GRANTED
            ) {
                Log.e("BLE", "Nedostaju potrebne dozvole za BLE skeniranje.")
                ActivityCompat.requestPermissions(
                    this,
                    arrayOf(
                        Manifest.permission.BLUETOOTH_SCAN,
                        Manifest.permission.BLUETOOTH_CONNECT
                    ),
                    101
                )
                return
            }
        }

        // Postavke skeniranja bez filtera (skeniranje svih uređaja)
        val scanSettings = ScanSettings.Builder()
            .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
            .build()

        try {
            bluetoothLeScanner.startScan(null, scanSettings, scanCallback)
            Log.d("BLE", "Skeniranje pokrenuto (bez filtera)!")
        } catch (e: Exception) {
            Log.e("BLE", "Greška prilikom pokretanja skeniranja: ${e.message}")
            return
        }

        // Automatsko zaustavljanje skeniranja nakon 10 sekundi
        Handler(Looper.getMainLooper()).postDelayed({
            bluetoothLeScanner.stopScan(scanCallback)
            Log.d("BLE", "Skeniranje završeno.")
            statusTextView.text = "Skeniranje završeno"
        }, 10000)
    }

    // Callback funkcija za skeniranje BLE uređaja
    private val scanCallback = object : ScanCallback() {
        override fun onScanResult(callbackType: Int, result: ScanResult) {
            Log.d("BLE", "Rezultat skeniranja dobiven")

            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
                if (ActivityCompat.checkSelfPermission(
                        this@MainActivity,
                        Manifest.permission.BLUETOOTH_CONNECT
                    ) != PackageManager.PERMISSION_GRANTED
                ) {
                    Log.e("BLE", "Dozvola za BLUETOOTH_CONNECT nije odobrena. Zatvaranje BLE veze preskočeno.")
                    return
                }
            }

            // Dohvaćanje imena uređaja, ako nije dostupno prikazujemo "Nepoznato"
            val foundDeviceName = result.device.name ?: "Nepoznato"
            Log.d("BLE", "Pronađen uređaj: $foundDeviceName - ${result.device.address}")

            // Ažuriraj UI s pronađenim uređajem
            runOnUiThread {
                statusTextView.text = "Pronađen: $foundDeviceName"
            }

            // Provjera je li pronađen naš uređaj "ESP32-Posadivac5000"
            if (foundDeviceName == "ESP32-Posadivac5000") {
                Log.d("BLE", "Pronađen traženi uređaj: ESP32-Posadivac5000, prekidam skeniranje...")
                bluetoothLeScanner.stopScan(this)

                runOnUiThread {
                    statusTextView.text = "Povezivanje s: $foundDeviceName"
                }

                connectToDevice(result.device)
            }
        }

        override fun onBatchScanResults(results: MutableList<ScanResult>) {
            Log.d("BLE", "Pristiglo više rezultata skeniranja (${results.size} uređaja).")
            for (result in results) {
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
                    if (ActivityCompat.checkSelfPermission(
                            this@MainActivity,
                            Manifest.permission.BLUETOOTH_CONNECT
                        ) != PackageManager.PERMISSION_GRANTED
                    ) {
                        Log.e("Bluetooth", "Dozvola za BLUETOOTH_CONNECT nije odobrena. Zatvaranje BLE veze preskočeno.")
                        return
                    }
                }

                val foundDeviceName = result.device.name ?: "Nepoznato"
                Log.d("BLE", "Pronađen uređaj u batch skeniranju: $foundDeviceName - ${result.device.address}")

                if (foundDeviceName == "ESP32-Posadivac5000") {
                    Log.d("BLE", "Pronađen traženi uređaj u batch skeniranju: ESP32-Posadivac5000")
                    bluetoothLeScanner.stopScan(this)
                    connectToDevice(result.device)
                    break
                }
            }
        }

        override fun onScanFailed(errorCode: Int) {
            Log.e("BLE", "Skeniranje nije uspjelo: $errorCode")
            runOnUiThread {
                statusTextView.text = "Skeniranje nije uspjelo: $errorCode"
            }
        }
    }

    // Povezivanje s pronađenim BLE uređajem
    private fun connectToDevice(device: BluetoothDevice) {
        Log.d("BLE", "Povezivanje s uređajem ${device.address}")
        statusTextView.text = "Povezivanje s uređajem..."

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            if (ActivityCompat.checkSelfPermission(
                    this,
                    Manifest.permission.BLUETOOTH_CONNECT
                ) != PackageManager.PERMISSION_GRANTED
            ) {
                Log.e("Bluetooth", "Dozvola za BLUETOOTH_CONNECT nije odobrena. Zatvaranje BLE veze preskočeno.")
                return
            }
        }
        bluetoothGatt = device.connectGatt(this, false, gattCallback)
    }

    // Callback za upravljanje BLE vezom
    private val gattCallback = object : BluetoothGattCallback() {
        override fun onConnectionStateChange(gatt: BluetoothGatt?, status: Int, newState: Int) {
            if (newState == BluetoothGatt.STATE_CONNECTED) {
                Log.d("BLE", "Povezano s uređajem")
                runOnUiThread { statusTextView.text = "Povezano s uređajem!" }
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
                    if (ActivityCompat.checkSelfPermission(
                            this@MainActivity,
                            Manifest.permission.BLUETOOTH_CONNECT
                        ) != PackageManager.PERMISSION_GRANTED
                    ) {
                        Log.e("Bluetooth", "Dozvola za BLUETOOTH_CONNECT nije odobrena. Zatvaranje BLE veze preskočeno.")
                        return
                    }
                }
                gatt?.discoverServices()
            } else if (newState == BluetoothGatt.STATE_DISCONNECTED) {
                Log.e("BLE", "Odspojen od uređaja")
                runOnUiThread { statusTextView.text = "Odspojeno od uređaja" }
            }
        }

        override fun onServicesDiscovered(gatt: BluetoothGatt?, status: Int) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                Log.d("BLE", "Servisi uspješno otkriveni")
                runOnUiThread { statusTextView.text = "Servisi pronađeni" }
            } else {
                Log.e("BLE", "Greška pri otkrivanju servisa: $status")
            }
        }
    }

    // Obrada rezultata traženja dozvola
    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode == 101) {
            if (grantResults.all { it == PackageManager.PERMISSION_GRANTED }) {
                Log.d("BLE", "Dozvole odobrene, pokrećem skeniranje")
                startBLEScan()
            } else {
                statusTextView.text = "Dozvole nisu odobrene"
                Log.e("BLE", "Korisnik nije odobrio Bluetooth dozvole")
            }
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            if (ActivityCompat.checkSelfPermission(
                    this,
                    Manifest.permission.BLUETOOTH_CONNECT
                ) != PackageManager.PERMISSION_GRANTED
            ) {
                Log.e("Bluetooth", "Dozvola za BLUETOOTH_CONNECT nije odobrena. Zatvaranje BLE veze preskočeno.")
                return
            }
        }
        bluetoothGatt?.close()
        Log.d("BLE", "BLE veza zatvorena")
    }
}
