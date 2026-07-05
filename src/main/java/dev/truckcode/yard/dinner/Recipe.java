package dev.truckcode.yard.dinner;

import jakarta.persistence.*;
import jakarta.validation.Valid;
import jakarta.validation.constraints.NotBlank;
import jakarta.validation.constraints.Size;

import java.util.List;
import java.util.stream.Collectors;

@Entity
@Table(name = "recipe")
public class Recipe {

    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    private Integer id;

    @NotBlank
    @Size(max = 150)
    private String title;

    // Every recipe is routed by this — no more slug/token split, one identifier
    // scheme for public and private recipes alike.
    @Column(length = 5, nullable = false, unique = true)
    private String token;

    // true = visible to everyone, ownerId is irrelevant. false = private,
    // ownerId must be set. Defaults false, the safe direction: a bug that
    // forgets to set this leaves a recipe under-shared, not leaked to everyone.
    @Column(name = "is_global", nullable = false)
    private boolean isGlobal;

    // Who made it. Household visibility is computed from this via a join to
    // app_user.group_id (see RecipeRepository), not stamped on the recipe —
    // so it stays correct if the owner later changes households.
    @Column(name = "owner_id")
    private Long ownerId;

    @Valid
    @Size(max = 50)
    @ElementCollection(fetch = FetchType.EAGER)
    @CollectionTable(name = "recipe_ingredient", joinColumns = @JoinColumn(name = "recipe_id"))
    @OrderColumn(name = "position")
    private List<Ingredient> ingredients;

    @Size(max = 50)
    @ElementCollection(fetch = FetchType.EAGER)
    @CollectionTable(name = "recipe_method_step", joinColumns = @JoinColumn(name = "recipe_id"))
    @OrderColumn(name = "position")
    @Column(name = "step")
    private List<@Size(max = 500) String> method;

    public Integer getId() { return id; }
    public void setId(Integer id) { this.id = id; }

    public String getTitle() { return title; }
    public void setTitle(String title) { this.title = title; }

    public String getToken() { return token; }
    public void setToken(String token) { this.token = token; }

    public boolean isGlobal() { return isGlobal; }
    public void setGlobal(boolean global) { isGlobal = global; }

    public Long getOwnerId() { return ownerId; }
    public void setOwnerId(Long ownerId) { this.ownerId = ownerId; }

    public List<Ingredient> getIngredients() { return ingredients; }
    public void setIngredients(List<Ingredient> ingredients) { this.ingredients = ingredients; }

    public String getIngredientNames() {
        return ingredients.stream().map(Ingredient::name).collect(Collectors.joining(","));
    }

    public List<String> getMethod() { return method; }
    public void setMethod(List<String> method) { this.method = method; }
}
