package org.gozaltech.nvdaremotecompanion.android;

public final class Event<T> {

    private final T content;
    private boolean consumed = false;

    public Event(T content) {
        this.content = content;
    }

    public T consume() {
        if (consumed) return null;
        consumed = true;
        return content;
    }

    public boolean isConsumed() { return consumed; }

    public T peek() {
        return content;
    }
}
