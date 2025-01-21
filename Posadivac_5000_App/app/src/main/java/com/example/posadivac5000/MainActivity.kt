package com.example.posadivac5000

import android.os.Bundle
import android.widget.Button
import android.widget.ProgressBar
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity


class MainActivity : AppCompatActivity(), BluetoothHelper.BluetoothListener {

    private lateinit var bluetoothHelper: BluetoothHelper

    private lateinit var statusTextView: TextView
    private lateinit var airTemperatureValue: TextView
    private lateinit var airHumidityProgress: ProgressBar
    private lateinit var airHumidityText: TextView
    private lateinit var soilHumidityProgress: ProgressBar
    private lateinit var soilHumidityValue: TextView
    private lateinit var scanButton: Button

    private lateinit var sendPromptButton: Button
    private lateinit var responseTextView: TextView

    override fun onCreate(savedInstanceState: Bundle?) {
    super.onCreate(savedInstanceState)
    setContentView(R.layout.activity_main)

    statusTextView = findViewById(R.id.statusTextView)
    airTemperatureValue = findViewById(R.id.air_temperature_value)
    airHumidityProgress = findViewById(R.id.air_humidity_progress)
    airHumidityText = findViewById(R.id.air_humidity_text)
    soilHumidityProgress = findViewById(R.id.soil_humidity_progress)
    soilHumidityValue = findViewById(R.id.soil_humidity_value)
    scanButton = findViewById(R.id.btn_scan)

    bluetoothHelper = BluetoothHelper(this, this)

    sendPromptButton = findViewById(R.id.btn_send_prompt)
    responseTextView = findViewById(R.id.chatgpt_response)

    sendPromptButton.setOnClickListener {
        val temperature = airTemperatureValue.text.toString().replace(" °C", "").toDoubleOrNull()
        val humidity = airHumidityText.text.toString().replace(" %", "").toIntOrNull()
        val soilMoisture = soilHumidityValue.text.toString().replace(" %", "").toIntOrNull()

        if (temperature != null && humidity != null && soilMoisture != null) {
            val query = "Koja biljka je najbolja za posaditi s temperaturom od $temperature°C, vlagom zraka $humidity% i vlagom tla $soilMoisture%?"
            sendQueryToChatGPT(query)
        } else {
            responseTextView.text = "Podaci nisu dostupni za slanje upita."
        }
    }

    scanButton.setOnClickListener {
        if (bluetoothHelper.isBluetoothEnabled()) {
            bluetoothHelper.startScan()
            statusTextView.text = "Skeniram uređaje..."
        } else {
            statusTextView.text = "Uključi Bluetooth!"
        }
        }
    }

    override fun onDeviceFound(deviceName: String) {
        runOnUiThread {
            statusTextView.text = "Pronađen: $deviceName"
        }
    }

    override fun onConnected() {
        runOnUiThread {
            statusTextView.text = "Spojen sa ESP32-Posadivac5000"
            scanButton.isEnabled = false
            scanButton.text = "Spojeno"
        }
    }

    override fun onDisconnected() {
        runOnUiThread {
            statusTextView.text = "Odspojen od ESP32"
            scanButton.isEnabled = true
            scanButton.text = "Skeniraj uređaje"
        }
    }

    override fun onDataReceived(data: String) {
        runOnUiThread {
            statusTextView.text = "Podaci primljeni!"
            parseAndDisplayData(data)
        }
    }

    override fun onError(message: String) {
        runOnUiThread {
            statusTextView.text = message
        }
    }

    override fun onScanCompleted(success: Boolean) {
        runOnUiThread {
            if (!success) statusTextView.text = "Uređaj nije pronađen"
        }
    }

    private fun parseAndDisplayData(data: String) {
        val jsonObject = org.json.JSONObject(data)
        val temperature = jsonObject.getDouble("temperature")
        val humidity = jsonObject.getInt("humidity")
        val soilMoisture = jsonObject.getInt("soil_moisture")

        airTemperatureValue.text = "$temperature °C"
        airHumidityProgress.progress = humidity
        airHumidityText.text = "$humidity %"
        soilHumidityProgress.progress = soilMoisture
        soilHumidityValue.text = "$soilMoisture %"
    }

    private fun sendQueryToChatGPT(query: String){

    }
}
