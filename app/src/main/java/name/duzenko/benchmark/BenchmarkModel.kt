package name.duzenko.benchmark

import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.GlobalScope
import kotlinx.coroutines.launch

class BenchmarkModel {

    var totalTests = 0
        private set

    var isRunning = false
        private set

    private var listener: BenchmarkListener? = null

    init {
        totalTests = getTestCount()
    }

    fun setListener(listener: BenchmarkListener) {
        this.listener = listener
    }

    fun runBenchmarks() {
        if (isRunning) return
        isRunning = true

        listener?.onBenchmarkStarted()

        GlobalScope.launch(Dispatchers.IO) {
            runAllMemoryBenchmarks(object : BenchmarkCallback {
                override fun onProgressUpdate(result: String) {
                    listener?.onProgressUpdated(result)
                }

                override fun onFinished() {
                    isRunning = false
                    listener?.onBenchmarkFinished()
                }
            })
        }
    }

    /**
     * A native method that is implemented by the 'myapplication' native library,
     * which is packaged with this application.
     */
    private external fun runAllMemoryBenchmarks(callback: BenchmarkCallback)
    private external fun getTestCount(): Int

    companion object {
        // Used to load the 'myapplication' library on application startup.
        init {
            System.loadLibrary("myapplication")
        }
    }
}

interface BenchmarkListener {
    fun onBenchmarkStarted()
    fun onProgressUpdated(result: String)
    fun onBenchmarkFinished()
}
