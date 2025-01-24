package com.example.posadivac5000

import android.os.Bundle
import android.util.Log
import android.widget.Button
import android.widget.ProgressBar
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import okhttp3.Call
import okhttp3.Callback
import okhttp3.MediaType
import okhttp3.OkHttpClient
import okhttp3.Request
import okhttp3.RequestBody
import okhttp3.Response
import org.json.JSONArray
import org.json.JSONObject
import java.io.IOException

var apiKey = "xxx"

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
            val query = "Koja biljka je najbolja za posaditi u zemlju s temperaturom zraka od $temperature°C, vlagom zraka $humidity% i vlagom tla $soilMoisture%?"
            sendQueryToChatGPT(query)
            //sendQueryToHuggingFace(query)
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
        var client = OkHttpClient()
        var json = JSONObject()

        json.put("model","gpt-3.5-turbo")
        json.put("messages", JSONArray().put(JSONObject().put("role", "user").put("content", query)))

        val requestBody = RequestBody.create(
            MediaType.parse("application/json; charset=utf-8"),
            json.toString()
        )

        val request = Request.Builder()
            .url("https://api.openai.com/v1/chat/completions")
            .addHeader("Authorization", "Bearer $apiKey")
            .post(requestBody)
            .build()

        client.newCall(request).enqueue(object : Callback {
            override fun onFailure(call: Call, e: IOException) {
                runOnUiThread {
                    responseTextView.text = "Greška prilikom dohvaćanja savjeta."
                }
            }

            override fun onResponse(call: Call, response: Response) {
                response.body()?.let {
                    val responseBody = it.string()
                    val jsonResponse = JSONObject(responseBody)
                    Log.d("API_RESPONSE", "Odgovor od API-ja: $responseBody")
                    val answer = jsonResponse.getJSONArray("choices").getJSONObject(0).getJSONObject("message").getString("content")

                    runOnUiThread {
                        responseTextView.text = "Savjet: $answer"
                    }
                }
            }
        })
    }

    /*private fun sendQueryToHuggingFace(query: String) {
        val apiKey = "xxx"
        val client = OkHttpClient()
        val json = JSONObject()

        try {
            json.put("inputs", query)
            Log.d("API_REQUEST", "Slanje upita: $query")
        } catch (e: Exception) {
            Log.e("API_REQUEST", "Greška pri kreiranju JSON upita: ${e.message}")
            responseTextView.text = "Greška pri kreiranju upita."
            return
        }

        val requestBody = RequestBody.create(
            MediaType.parse("application/json; charset=utf-8"),
            json.toString()
        )

        val request = Request.Builder()
            .url("https://api-inference.huggingface.co/models/gpt2")
            .addHeader("Authorization", "Bearer $apiKey")
            .post(requestBody)
            .build()

        Log.d("API_REQUEST", "HTTP zahtjev poslan na URL: ${request.url()}")

        client.newCall(request).enqueue(object : Callback {
            override fun onFailure(call: Call, e: IOException) {
                Log.e("API_RESPONSE", "Greška prilikom dohvaćanja savjeta: ${e.message}")
                runOnUiThread {
                    responseTextView.text = "Greška prilikom dohvaćanja savjeta."
                }
            }

            override fun onResponse(call: Call, response: Response) {
                Log.d("API_RESPONSE", "HTTP kod odgovora: ${response.code()}")

                response.body()?.let {
                    val responseBody = it.string()
                    Log.d("API_RESPONSE", "Primljen odgovor: $responseBody")

                    try {
                        val jsonResponse = JSONArray(responseBody)
                        val answer = jsonResponse.getJSONObject(0).getString("generated_text")
                        Log.d("API_RESPONSE", "Parsirani odgovor: $answer")

                        runOnUiThread {
                            responseTextView.text = "Savjet: $answer"
                        }
                    } catch (e: Exception) {
                        Log.e("API_RESPONSE", "Greška pri parsiranju odgovora: ${e.message}")
                        runOnUiThread {
                            responseTextView.text = "Greška u odgovoru API-ja."
                        }
                    }
                } ?: run {
                    Log.e("API_RESPONSE", "Odgovor tijela je prazan!")
                    runOnUiThread {
                        responseTextView.text = "Prazan odgovor API-ja."
                    }
                }
            }
        })
    }*/
}



