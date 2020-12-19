package org.uoyabause.android.game

class LeaderBoard(val title: String, val id: String)

abstract interface GameUiEvent {
    abstract fun onNewRecord(leaderBoardId: String)
}

abstract class BaseGame {

    var leaderBoards: MutableList<LeaderBoard>? = null

    lateinit var uievent: GameUiEvent
    fun setUiEvent(uievent: GameUiEvent) {
        this.uievent = uievent
    }
    abstract fun onBackUpUpdated(before: ByteArray, after: ByteArray)
}
