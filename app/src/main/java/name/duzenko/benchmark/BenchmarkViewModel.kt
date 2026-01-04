package name.duzenko.benchmark

import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.ViewModel

class BenchmarkViewModel : ViewModel(), BenchmarkListener {

    private val model = BenchmarkModel()

    private val _results = MutableLiveData<List<String>>(emptyList())
    val results: LiveData<List<String>> = _results

    private val _progress = MutableLiveData<Pair<Int, Int>?>(null)
    val progress: LiveData<Pair<Int, Int>?> = _progress

    private val _isRunning = MutableLiveData(false)
    val isRunning: LiveData<Boolean> = _isRunning

    val coreCount: Int
        get() = model.getCoreCount()

    val totalTests: Int
        get() = model.totalTests

    init {
        model.setListener(this)
    }

    fun runBenchmarks() {
        _results.value = emptyList()
        model.runBenchmarks()
    }

    override fun onBenchmarkStarted() {
        _isRunning.postValue(true)
    }

    override fun onProgressUpdated(result: String) {
        val currentResults = _results.value ?: emptyList()
        _results.postValue(currentResults + result)
    }

    override fun onBenchmarkFinished() {
        _isRunning.postValue(false)
    }

    override fun onCleared() {
        super.onCleared()
        model.cancel()
    }
}
