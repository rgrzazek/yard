package dev.truckcode.yard.dinner;

import jakarta.validation.constraints.NotBlank;
import jakarta.validation.constraints.Size;

import java.util.ArrayList;
import java.util.List;

// Plain form-backing object, not the @Entity — a form must never bind directly
// onto Recipe, since that would let a request set fields like group/token/id
// that only the server is allowed to decide.
public class RecipeForm {

    @NotBlank
    @Size(max = 150)
    private String title;

    @Size(max = 50)
    private List<@NotBlank @Size(max = 100) String> ingredients = new ArrayList<>();

    @Size(max = 50)
    private List<@NotBlank @Size(max = 500) String> steps = new ArrayList<>();

    public String getTitle() { return title; }
    public void setTitle(String title) { this.title = title; }

    public List<String> getIngredients() { return ingredients; }
    public void setIngredients(List<String> ingredients) { this.ingredients = ingredients; }

    public List<String> getSteps() { return steps; }
    public void setSteps(List<String> steps) { this.steps = steps; }
}
