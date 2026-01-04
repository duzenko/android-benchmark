package name.duzenko.benchmark

import android.graphics.Typeface
import android.os.Bundle
import android.text.TextUtils
import android.view.GestureDetector
import android.view.Gravity
import android.view.MotionEvent
import android.view.View
import android.widget.TableRow
import android.widget.TextView
import android.widget.Toast
import androidx.activity.viewModels
import androidx.appcompat.app.AppCompatActivity
import androidx.core.widget.TextViewCompat
import name.duzenko.benchmark.databinding.ActivityMainBinding
import java.text.DecimalFormat

class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding
    private val viewModel: BenchmarkViewModel by viewModels()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        binding.runBenchmarksButton.setOnClickListener {
            viewModel.runBenchmarks()
        }

        binding.coreCountText.text = "Cores: ${viewModel.coreCount}"

        observeViewModel()
    }

    private fun observeViewModel() {
        viewModel.isRunning.observe(this) { isRunning ->
            binding.runBenchmarksButton.isEnabled = !isRunning
            binding.progressBar.visibility = if (isRunning) View.VISIBLE else View.GONE
            binding.progressText.visibility = if (isRunning) View.VISIBLE else View.GONE
        }

        viewModel.results.observe(this) { results ->
            binding.resultsTable.visibility = if (results.isNotEmpty()) View.VISIBLE else View.GONE
            // Clear previous results, keeping the header row
            while (binding.resultsTable.childCount > 1) {
                binding.resultsTable.removeViewAt(1)
            }

            results.forEachIndexed { index, result ->
                addTableRow(result, index + 1, viewModel.totalTests)
            }
        }

        viewModel.progress.observe(this) { progress ->
            if (progress != null) {
                binding.progressText.text = "${progress.first}/${progress.second}"
            }
        }
    }

    private fun addTableRow(result: String, completed: Int, total: Int) {
        val parts = result.split("|")
        val testName = parts[0]

        if (testName.startsWith(SECTION_PREFIX)) {
            addSectionHeader(testName)
        } else {
            addResultRow(parts)
        }
    }

    private fun addSectionHeader(testName: String) {
        val sectionName = testName
            .removePrefix(SECTION_PREFIX)
            .removeSuffix(SECTION_SUFFIX)

        val tableRow = TableRow(this)
        val sectionView = TextView(this).apply {
            text = "\n$sectionName"
            textSize = resources.getDimension(R.dimen.text_size_section)
            setTypeface(null, Typeface.BOLD)
            gravity = Gravity.CENTER_HORIZONTAL
        }
        val layoutParams = TableRow.LayoutParams(TableRow.LayoutParams.MATCH_PARENT, TableRow.LayoutParams.WRAP_CONTENT)
        layoutParams.span = 3
        tableRow.addView(sectionView, layoutParams)
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

        val testNameView = createTextView(testName).apply {
            layoutParams = TableRow.LayoutParams(0, TableRow.LayoutParams.WRAP_CONTENT, 2f)
            ellipsize = TextUtils.TruncateAt.END
            maxLines = 1
        }

        val performanceView = createTextView(performanceString).apply {
            layoutParams = TableRow.LayoutParams(0, TableRow.LayoutParams.WRAP_CONTENT, 1f)
            ellipsize = TextUtils.TruncateAt.END
            maxLines = 1
        }

        val bandwidthView = createTextView(bandwidthString).apply {
            layoutParams = TableRow.LayoutParams(0, TableRow.LayoutParams.WRAP_CONTENT, 1f)
            ellipsize = TextUtils.TruncateAt.END
            maxLines = 1
        }

        tableRow.addView(testNameView)
        tableRow.addView(performanceView)
        tableRow.addView(bandwidthView)

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
            setPadding(8, 0, 8, 0)
            textSize = resources.getDimension(R.dimen.text_size_result)
            TextViewCompat.setAutoSizeTextTypeWithDefaults(this, TextViewCompat.AUTO_SIZE_TEXT_TYPE_UNIFORM)
        }
    }

    companion object {
        private const val SECTION_PREFIX = "---"
        private const val SECTION_SUFFIX = "---"
        private val SIGNIFICANT_DIGITS_FORMAT = DecimalFormat("@@@")
    }
}
