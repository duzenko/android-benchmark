package name.duzenko.benchmark

interface BenchmarkView {
    fun showProgress(total: Int)
    fun addTableRow(result: String, completed: Int, total: Int)
    fun hideProgress()
}

class BenchmarkController(
    private val model: BenchmarkModel,
    private val view: BenchmarkView
) {
    private var completedTests = 0

    init {
        model.setListener(object : BenchmarkListener {
            override fun onBenchmarkStarted() {
                completedTests = 0
                view.showProgress(model.totalTests)
            }

            override fun onProgressUpdated(result: String) {
                completedTests++
                view.addTableRow(result, completedTests, model.totalTests)
            }

            override fun onBenchmarkFinished() {
                view.hideProgress()
            }
        })
    }

    fun onRunBenchmarksClicked() {
        model.runBenchmarks()
    }
}
