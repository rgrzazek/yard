package dev.truckcode.yard.dinner;

import tools.jackson.core.type.TypeReference;
import tools.jackson.databind.ObjectMapper;
import org.springframework.core.io.ClassPathResource;
import java.io.IOException;
import java.util.Collections;
import java.util.List;

public class JsonRecipeRepository {

    private final List<Recipe> recipes;

    public JsonRecipeRepository(ObjectMapper objectMapper) throws IOException {
        var resource = new ClassPathResource("data/recipes.json");
        recipes = objectMapper.readValue(resource.getInputStream(), new TypeReference<>() {});
    }


    public List<Recipe> findAll() {
        return Collections.unmodifiableList(recipes);
    }
}
