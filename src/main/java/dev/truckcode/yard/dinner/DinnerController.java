package dev.truckcode.yard.dinner;

import dev.truckcode.yard.auth.CurrentUserService;
import jakarta.validation.Valid;
import org.springframework.stereotype.Controller;
import org.springframework.security.core.Authentication;
import org.springframework.ui.Model;
import org.springframework.validation.BindingResult;
import org.springframework.web.bind.annotation.*;

@Controller
public class DinnerController {

    private final RecipeService recipeService;
    private final CurrentUserService currentUserService;

    public DinnerController(RecipeService recipeService, CurrentUserService currentUserService) {
        this.recipeService = recipeService;
        this.currentUserService = currentUserService;
    }

    // Same redirect whether the token is wrong or just belongs to someone
    // else's household — never reveal which, so the URL can't be used to probe existence.
    @GetMapping("/dinner/{token}")
    public String recipe(@PathVariable String token, Authentication authentication, Model model) {
        var myGroupId = currentUserService.currentGroupId(authentication).orElse(null);
        var recipe = recipeService.getRecipe(token, myGroupId);
        if (recipe.isEmpty()) {
            return "redirect:/dinner";
        }
        model.addAttribute("recipe", recipe.get());
        return "recipe";
    }

    @GetMapping("/dinner")
    public String dinnerList(Authentication authentication, Model model) {
        var myGroupId = currentUserService.currentGroupId(authentication).orElse(null);
        var recipes = recipeService.getVisibleRecipes(myGroupId);
        model.addAttribute("recipes", recipes);
        model.addAttribute("ingredientNames", recipeService.getAllIngredientNames(myGroupId));
        model.addAttribute("tonightsRecipe", recipeService.getTodaysRecipe());
        model.addAttribute("hasCustomRecipes", recipes.stream().anyMatch(r -> !r.isGlobal()));
        return "dinner";
    }

    @GetMapping("/dinner/new")
    public String newRecipeForm(Model model) {
        if (!model.containsAttribute("recipeForm")) {
            model.addAttribute("recipeForm", new RecipeForm());
        }
        return "recipe-form";
    }

    @PostMapping("/dinner/save")
    public String save(@Valid @ModelAttribute("recipeForm") RecipeForm form, BindingResult bindingResult,
                        Authentication authentication, Model model) {
        if (bindingResult.hasErrors()) {
            return "recipe-form";
        }

        var groupId = currentUserService.currentGroupId(authentication).orElse(null);
        if (groupId == null) {
            return "redirect:/login";
        }

        try {
            var saved = recipeService.saveForGroup(form, groupId);
            return "redirect:/dinner/" + saved.getToken();
        } catch (RecipeLimitExceededException e) {
            model.addAttribute("limitError", e.getMessage());
            return "recipe-form";
        }
    }
}
