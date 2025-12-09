package name.duzenko.benchmark

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.view.View
import android.widget.TableRow
import android.widget.TextView
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
            val elementsPerSecond = parts[1].toDouble()
            val bandwidth = parts[2].toDouble()

            val significantDigitsFormat = DecimalFormat("@@@")

            val performanceString = if (elementsPerSecond > 1e9) {
                "${significantDigitsFormat.format(elementsPerSecond / 1e9)} B/s"
            } else {
                "${significantDigitsFormat.format(elementsPerSecond / 1e6)} M/s"
            }

            val bandwidthString: String
            if (bandwidth >= 1024) {
                val bandwidthInGB = bandwidth / 1024.0
                bandwidthString = "${significantDigitsFormat.format(bandwidthInGB)} GB/s"
            } else {
                bandwidthString = "${significantDigitsFormat.format(bandwidth)} MB/s"
            }

            val tableRow = TableRow(this)

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