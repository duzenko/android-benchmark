package name.duzenko.benchmark

interface BenchmarkCallback {
    fun onProgressUpdate(result: String)
    fun onFinished()
}
