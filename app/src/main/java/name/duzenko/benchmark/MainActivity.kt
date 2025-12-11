package name.duzenko.benchmark

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.view.GestureDetector
import android.view.MotionEvent
import android.view.View
import android.widget.TableRow
import android.widget.TextView
import android.widget.Toast
import name.duzenko.benchmark.databinding.ActivityMainBinding
import java.text.DecimalFormat

class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding
    private lateinit var controller: BenchmarkController

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        val model = BenchmarkModel()
        controller = BenchmarkController(model, this)

        binding.runBenchmarksButton.setOnClickListener {
            controller.onRunBenchmarksClicked()
        }

        binding.resultsTable.visibility = View.GONE
    }

    fun showProgress(total: Int) {
        runOnUiThread {
            binding.runBenchmarksButton.isEnabled = false
            binding.progressBar.visibility = View.VISIBLE
            binding.progressText.visibility = View.VISIBLE
            binding.progressText.text = "0/$total"
            binding.resultsTable.visibility = View.VISIBLE

            // Clear previous results, keeping the header row
            while (binding.resultsTable.childCount > 1) {
                binding.resultsTable.removeViewAt(1)
            }
        }
    }

    fun addTableRow(result: String, completed: Int, total: Int) {
        runOnUiThread {
            binding.progressText.text = "$completed/$total"

            val parts = result.split("|")
            val testName = parts[0]
            val numElements = parts[1].toLong()
            val elementSize = parts[2].toInt()
            val durationMs = parts[3].toDouble()
            val repetitions = parts[4].toInt()

            val totalBytesProcessed = numElements * elementSize * repetitions
            val bandwidth = (totalBytesProcessed / (1024.0 * 1024.0)) / (durationMs / 1000.0)
            val performanceMetric = (numElements * repetitions) / (durationMs / 1000.0)

            val dataSize = numElements * elementSize
            val dataSizeInMB = dataSize / (1024.0 * 1024.0)
            val detailsFormat = DecimalFormat("#,##0.##")
            val detailsString = result;//"Size: ${detailsFormat.format(dataSizeInMB)} MB, Time: ${detailsFormat.format(durationMs)} ms"

            val significantDigitsFormat = DecimalFormat("@@@")

            val performanceString: String
            if (testName.startsWith("memset")) {
                if (performanceMetric > 1e9) {
                    performanceString = "${significantDigitsFormat.format(performanceMetric / 1e9)} GB/s"
                } else {
                    performanceString = "${significantDigitsFormat.format(performanceMetric / 1e6)} MB/s"
                }
            } else {
                if (performanceMetric > 1e9) {
                    performanceString = "${significantDigitsFormat.format(performanceMetric / 1e9)} GE/s"
                } else {
                    performanceString = "${significantDigitsFormat.format(performanceMetric / 1e6)} ME/s"
                }
            }

            val bandwidthString: String
            if (bandwidth >= 1024) {
                val bandwidthInGB = bandwidth / 1024.0
                bandwidthString = "${significantDigitsFormat.format(bandwidthInGB)} GB/s"
            } else {
                bandwidthString = "${significantDigitsFormat.format(bandwidth)} MB/s"
            }

            val tableRow = TableRow(this)
            tableRow.tag = detailsString

            val gestureDetector = GestureDetector(this, object : GestureDetector.SimpleOnGestureListener() {
                override fun onDoubleTap(e: MotionEvent): Boolean {
                    Toast.makeText(this@MainActivity, tableRow.tag.toString(), Toast.LENGTH_LONG).show()
                    return true
                }
            })

            tableRow.setOnTouchListener { _, event ->
                gestureDetector.onTouchEvent(event)
                true
            }

            val testNameView = TextView(this).apply {
                text = testName
                textSize = 16f
            }
            tableRow.addView(testNameView)

            val performanceView = TextView(this).apply {
                text = performanceString
                textSize = 16f
            }
            tableRow.addView(performanceView)

            val bandwidthView = TextView(this).apply {
                text = bandwidthString
                textSize = 16f
            }
            tableRow.addView(bandwidthView)

            binding.resultsTable.addView(tableRow)
        }
    }

    fun hideProgress() {
        runOnUiThread {
            binding.progressBar.visibility = View.GONE
            binding.progressText.visibility = View.GONE
            binding.runBenchmarksButton.isEnabled = true
        }
    }
}