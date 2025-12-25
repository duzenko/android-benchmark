package name.duzenko.benchmark

import android.graphics.Typeface
import android.os.Bundle
import android.view.GestureDetector
import android.view.MotionEvent
import android.view.View
import android.widget.TableRow
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import name.duzenko.benchmark.databinding.ActivityMainBinding
import java.text.DecimalFormat

class MainActivity : AppCompatActivity(), BenchmarkView {

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

    override fun showProgress(total: Int) {
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

    override fun addTableRow(result: String, completed: Int, total: Int) {
        runOnUiThread {
            binding.progressText.text = "$completed/$total"

            val parts = result.split("|")
            val testName = parts[0]

            if (testName.startsWith(SECTION_PREFIX)) {
                addSectionHeader(testName)
            } else {
                addResultRow(parts)
            }
        }
    }

    private fun addSectionHeader(testName: String) {
        val sectionName = testName
            .removePrefix(SECTION_PREFIX)
            .removeSuffix(SECTION_SUFFIX)

        val tableRow = TableRow(this)
        val sectionView = TextView(this).apply {
            text = "\n$sectionName"
            textSize = TEXT_SIZE_SECTION
            setTypeface(null, Typeface.BOLD)
        }
        tableRow.addView(sectionView)
        binding.resultsTable.addView(tableRow)
    }

    private fun addResultRow(parts: List<String>) {
        val testName = parts[0]
        val numElements = parts[1].toLong()
        val elementSize = parts[2].toInt()
        val durationMs = parts[3].toDouble()
        val repetitions = parts[4].toInt()

        val performanceString = formatPerformance(testName, numElements, repetitions, durationMs)
        val bandwidthString = formatBandwidth(numElements, elementSize, repetitions, durationMs)

        val tableRow = createClickableRow(parts.joinToString("|"))
        tableRow.addView(createTextView(testName))
        tableRow.addView(createTextView(performanceString))
        tableRow.addView(createTextView(bandwidthString))

        binding.resultsTable.addView(tableRow)
    }

    private fun formatPerformance(
        testName: String,
        numElements: Long,
        repetitions: Int,
        durationMs: Double
    ): String {
        val performanceMetric = (numElements * repetitions) / (durationMs / 1000.0)

        return if (testName.startsWith("memset")) {
            formatWithUnit(performanceMetric, "B/s")
        } else {
            formatWithUnit(performanceMetric, "E/s")
        }
    }

    private fun formatBandwidth(
        numElements: Long,
        elementSize: Int,
        repetitions: Int,
        durationMs: Double
    ): String {
        val totalBytesProcessed = numElements * elementSize * repetitions
        val bandwidth = (totalBytesProcessed / (1024.0 * 1024.0)) / (durationMs / 1000.0)

        return if (bandwidth >= 1024) {
            "${SIGNIFICANT_DIGITS_FORMAT.format(bandwidth / 1024.0)} GB/s"
        } else {
            "${SIGNIFICANT_DIGITS_FORMAT.format(bandwidth)} MB/s"
        }
    }

    private fun formatWithUnit(value: Double, baseUnit: String): String {
        return if (value > 1e9) {
            "${SIGNIFICANT_DIGITS_FORMAT.format(value / 1e9)} G$baseUnit"
        } else {
            "${SIGNIFICANT_DIGITS_FORMAT.format(value / 1e6)} M$baseUnit"
        }
    }

    private fun createClickableRow(detailsString: String): TableRow {
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

        return tableRow
    }

    private fun createTextView(text: String): TextView {
        return TextView(this).apply {
            this.text = text
            textSize = TEXT_SIZE_RESULT
        }
    }

    override fun hideProgress() {
        runOnUiThread {
            binding.progressBar.visibility = View.GONE
            binding.progressText.visibility = View.GONE
            binding.runBenchmarksButton.isEnabled = true
        }
    }

    companion object {
        private const val SECTION_PREFIX = "---"
        private const val SECTION_SUFFIX = "---"
        private const val TEXT_SIZE_SECTION = 18f
        private const val TEXT_SIZE_RESULT = 16f
        private val SIGNIFICANT_DIGITS_FORMAT = DecimalFormat("@@@")
    }
}
