package dev.truckcode.yard.dinner;

import com.fasterxml.jackson.databind.ObjectMapper;
import org.springframework.boot.CommandLineRunner;
import org.springframework.stereotype.Component;

@Component
public class RecipeSeeder implements CommandLineRunner {

    private final RecipeRepository recipeRepository;
    private final RecipeService recipeService;
    private final ObjectMapper objectMapper;

    public RecipeSeeder(RecipeRepository recipeRepository, RecipeService recipeService, ObjectMapper objectMapper) {
        this.recipeRepository = recipeRepository;
        this.recipeService = recipeService;
        this.objectMapper = objectMapper;
    }

    @Override
    public void run(String... args) throws Exception {
        if (recipeRepository.count() > 0) return;

        var json = new JsonRecipeRepository(objectMapper);
        var recipes = json.findAll();
        recipes.forEach(r -> {
            r.setId(null);
            r.setGlobal(true);
            r.setToken(recipeService.generateUniqueToken());
        });
        recipeRepository.saveAll(recipes);
    }
}
