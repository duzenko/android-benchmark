package name.duzenko.benchmark

import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.cancel
import kotlinx.coroutines.launch

interface BenchmarkListener {
    fun onBenchmarkStarted()
    fun onProgressUpdated(result: String)
    fun onBenchmarkFinished()
}

class BenchmarkModel {

    var totalTests: Int = 0
        private set

    var isRunning: Boolean = false
        private set

    private var listener: BenchmarkListener? = null
    private val scope = CoroutineScope(SupervisorJob() + Dispatchers.IO)

    init {
        totalTests = getTestCount()
    }

    fun setListener(listener: BenchmarkListener) {
        this.listener = listener
    }

    fun getCoreCount(): Int {
        return getHardwareConcurrency()
    }

    fun runBenchmarks() {
        if (isRunning) return
        isRunning = true

        listener?.onBenchmarkStarted()

        scope.launch {
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

    fun cancel() {
        scope.cancel()
        isRunning = false
    }

    private external fun runAllMemoryBenchmarks(callback: BenchmarkCallback)
    private external fun getTestCount(): Int
    private external fun getHardwareConcurrency(): Int

    companion object {
        init {
            System.loadLibrary("myapplication")
        }
    }
}
