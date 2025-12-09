package name.duzenko.benchmark

class BenchmarkController(private val model: BenchmarkModel, private val view: MainActivity) {

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
