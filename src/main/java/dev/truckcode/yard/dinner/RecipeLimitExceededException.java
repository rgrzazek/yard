package dev.truckcode.yard.dinner;

public class RecipeLimitExceededException extends RuntimeException {
    public RecipeLimitExceededException(String message) {
        super(message);
    }
}
