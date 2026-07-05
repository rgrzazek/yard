package dev.truckcode.yard.dinner;

import org.springframework.stereotype.Service;

import java.security.SecureRandom;
import java.time.LocalDate;
import java.util.ArrayList;
import java.util.List;
import java.util.Optional;
import java.util.stream.Collectors;

@Service
public class RecipeService {

    private static final String TOKEN_CHARS = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    private static final int TOKEN_LENGTH = 5;
    private static final int MAX_RECIPES_PER_GROUP = 100;

    private final RecipeRepository recipeRepository;
    private final SecureRandom random = new SecureRandom();

    public RecipeService(RecipeRepository recipeRepository) {
        this.recipeRepository = recipeRepository;
    }

    public Recipe getTodaysRecipe() {
        List<Recipe> global = recipeRepository.findAllVisibleTo(null);
        int index = (int) (LocalDate.now().toEpochDay() % global.size());
        return global.get(index);
    }

    // myGroupId must come from the caller's own session (CurrentUserService),
    // never a request param.
    public Optional<Recipe> getRecipe(String token, Long myGroupId) {
        return recipeRepository.findVisibleByToken(token, myGroupId);
    }

    public List<Recipe> getVisibleRecipes(Long myGroupId) {
        return recipeRepository.findAllVisibleTo(myGroupId);
    }

    public List<String> getAllIngredientNames(Long myGroupId) {
        return getVisibleRecipes(myGroupId).stream()
                .flatMap(r -> r.getIngredients().stream())
                .filter(i -> i.getIngredientType() != IngredientType.STAPLE)
                .map(Ingredient::name)
                .distinct()
                .sorted()
                .collect(Collectors.toList());
    }

    public Recipe saveForGroup(RecipeForm form, Long groupId) {
        if (recipeRepository.countByGroupId(groupId) >= MAX_RECIPES_PER_GROUP) {
            throw new RecipeLimitExceededException("Your household has reached the " + MAX_RECIPES_PER_GROUP + " recipe limit.");
        }

        var recipe = new Recipe();
        recipe.setTitle(form.getTitle());
        recipe.setIngredients(mapIngredients(form.getIngredients()));
        recipe.setMethod(new ArrayList<>(form.getSteps()));
        recipe.setGlobal(false);
        recipe.setGroupId(groupId);
        recipe.setToken(generateUniqueToken());

        return recipeRepository.save(recipe);
    }

    public boolean canModify(Recipe recipe, Long myGroupId) {
        return !recipe.isGlobal() && myGroupId != null && myGroupId.equals(recipe.getGroupId());
    }

    public Optional<Recipe> getEditableRecipe(String token, Long myGroupId) {
        if (myGroupId == null) return Optional.empty();
        return recipeRepository.findEditableByToken(token, myGroupId);
    }

    public boolean updateForGroup(String token, RecipeForm form, Long myGroupId) {
        var recipeOpt = getEditableRecipe(token, myGroupId);
        if (recipeOpt.isEmpty()) return false;

        var recipe = recipeOpt.get();
        recipe.setTitle(form.getTitle());
        recipe.setIngredients(mapIngredients(form.getIngredients()));
        recipe.setMethod(new ArrayList<>(form.getSteps()));
        recipeRepository.save(recipe);
        return true;
    }

    public boolean deleteForGroup(String token, Long myGroupId) {
        var recipeOpt = getEditableRecipe(token, myGroupId);
        if (recipeOpt.isEmpty()) return false;
        recipeRepository.delete(recipeOpt.get());
        return true;
    }

    private List<Ingredient> mapIngredients(List<String> lines) {
        return lines.stream()
                .map(line -> new Ingredient(line, null, line, null))
                .collect(Collectors.toList());
    }

    // Package-visible so RecipeSeeder can mint tokens for the curated set too.
    String generateUniqueToken() {
        String candidate;
        do {
            var sb = new StringBuilder(TOKEN_LENGTH);
            for (int i = 0; i < TOKEN_LENGTH; i++) {
                sb.append(TOKEN_CHARS.charAt(random.nextInt(TOKEN_CHARS.length())));
            }
            candidate = sb.toString();
        } while (recipeRepository.existsByToken(candidate));
        return candidate;
    }
}
