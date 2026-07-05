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
    private static final int MAX_RECIPES_PER_OWNER = 100;

    private final RecipeRepository recipeRepository;
    private final SecureRandom random = new SecureRandom();

    public RecipeService(RecipeRepository recipeRepository) {
        this.recipeRepository = recipeRepository;
    }

    public Recipe getTodaysRecipe() {
        List<Recipe> global = recipeRepository.findAllVisibleTo(null, null);
        int index = (int) (LocalDate.now().toEpochDay() % global.size());
        return global.get(index);
    }

    // myUserId/myGroupId must come from the caller's own session
    // (CurrentUserService), never a request param.
    public Optional<Recipe> getRecipe(String token, Long myUserId, Long myGroupId) {
        return recipeRepository.findVisibleByToken(token, myUserId, myGroupId);
    }

    public List<Recipe> getVisibleRecipes(Long myUserId, Long myGroupId) {
        return recipeRepository.findAllVisibleTo(myUserId, myGroupId);
    }

    public List<String> getAllIngredientNames(Long myUserId, Long myGroupId) {
        return getVisibleRecipes(myUserId, myGroupId).stream()
                .flatMap(r -> r.getIngredients().stream())
                .filter(i -> i.getIngredientType() != IngredientType.STAPLE)
                .map(Ingredient::name)
                .distinct()
                .sorted()
                .collect(Collectors.toList());
    }

    public Recipe saveForOwner(RecipeForm form, Long ownerId) {
        if (recipeRepository.countByOwnerId(ownerId) >= MAX_RECIPES_PER_OWNER) {
            throw new RecipeLimitExceededException("You've reached the " + MAX_RECIPES_PER_OWNER + " recipe limit.");
        }

        var recipe = new Recipe();
        recipe.setTitle(form.getTitle());
        recipe.setIngredients(mapIngredients(form.getIngredients()));
        recipe.setMethod(new ArrayList<>(form.getSteps()));
        recipe.setGlobal(false);
        recipe.setOwnerId(ownerId);
        recipe.setToken(generateUniqueToken());

        return recipeRepository.save(recipe);
    }

    public boolean canModify(Recipe recipe, Long myUserId) {
        return !recipe.isGlobal() && myUserId != null && myUserId.equals(recipe.getOwnerId());
    }

    public Optional<Recipe> getEditableRecipe(String token, Long myUserId) {
        if (myUserId == null) return Optional.empty();
        return recipeRepository.findEditableByToken(token, myUserId);
    }

    public boolean updateForOwner(String token, RecipeForm form, Long myUserId) {
        var recipeOpt = getEditableRecipe(token, myUserId);
        if (recipeOpt.isEmpty()) return false;

        var recipe = recipeOpt.get();
        recipe.setTitle(form.getTitle());
        recipe.setIngredients(mapIngredients(form.getIngredients()));
        recipe.setMethod(new ArrayList<>(form.getSteps()));
        recipeRepository.save(recipe);
        return true;
    }

    public boolean deleteForOwner(String token, Long myUserId) {
        var recipeOpt = getEditableRecipe(token, myUserId);
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
