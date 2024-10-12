package dev.xyze.silene

import android.content.Intent
import android.os.Build
import android.os.Bundle
import android.os.FileUtils
import android.view.View
import android.widget.Button
import android.widget.TextView
import android.widget.Toast
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.lifecycleScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import java.io.File
import java.io.InputStream
import java.io.OutputStream

class FileSelectActivity : AppCompatActivity() {
    private val applicationFile by lazy {
        File(this.filesDir, "app.apk")
    }

    val onSelect = registerForActivityResult(ActivityResultContracts.GetContent()) { uri ->
        if (uri != null) {
            val fileIn = contentResolver.openInputStream(uri)
            val fileOut = applicationFile

            this.activateCopy()

            val activity = this
            lifecycleScope.launch(Dispatchers.IO) {
                fileIn?.use { fileStream ->
                    fileOut.outputStream().use { outStream ->
                        activity.copyFile(fileStream, outStream)

                        runOnUiThread {
                            activity.onCopyFinished()
                        }
                    }
                }
            }
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.select_layout)

        val selectButton: Button = findViewById(R.id.select_button)
        selectButton.setOnClickListener {
            onSelect.launch("application/vnd.android.package-archive")
        }

        if (applicationFile.exists()) {
            val previousButton: Button = findViewById(R.id.use_last_button)
            previousButton.isEnabled = true
            previousButton.visibility = View.VISIBLE

            previousButton.setOnClickListener {
                onCopyFinished()
            }
        }
    }

    private fun onCopyFinished() {
        if (!applicationFile.exists()) {
            Toast.makeText(this, R.string.select_failed, Toast.LENGTH_SHORT).show()
            return
        }

        val launchIntent = Intent(this, SileneActivity::class.java)
        launchIntent.flags = Intent.FLAG_ACTIVITY_CLEAR_TASK or Intent.FLAG_ACTIVITY_NEW_TASK

        startActivity(launchIntent)
    }

    private fun activateCopy() {
        val selectButton: Button = findViewById(R.id.select_button)
        selectButton.isEnabled = false

        val infoText: TextView = findViewById(R.id.select_details)
        infoText.text = this.getString(R.string.select_copying)
    }

    private fun copyFile(inputStream: InputStream, outputStream: OutputStream) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            FileUtils.copy(inputStream, outputStream)
        } else {
            inputStream.use { input ->
                outputStream.use { output ->
                    val buffer = ByteArray(4 * 1024)
                    while (true) {
                        val byteCount = input.read(buffer)
                        if (byteCount < 0) break
                        output.write(buffer, 0, byteCount)
                    }
                    output.flush()
                }
            }
        }
    }
}