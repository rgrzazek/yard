package dev.truckcode.yard.dinner;

import dev.truckcode.yard.auth.CurrentUserService;
import jakarta.validation.Valid;
import org.springframework.stereotype.Controller;
import org.springframework.security.core.Authentication;
import org.springframework.ui.Model;
import org.springframework.validation.BindingResult;
import org.springframework.web.bind.annotation.*;

import java.util.ArrayList;
import java.util.stream.Collectors;

@Controller
public class DinnerController {

    private final RecipeService recipeService;
    private final CurrentUserService currentUserService;

    public DinnerController(RecipeService recipeService, CurrentUserService currentUserService) {
        this.recipeService = recipeService;
        this.currentUserService = currentUserService;
    }

    // Same redirect whether the token is wrong or just not visible to this
    // user — never reveal which, so the URL can't be used to probe existence.
    @GetMapping("/dinner/{token}")
    public String recipe(@PathVariable String token, Authentication authentication, Model model) {
        var myUserId = currentUserService.currentUserId(authentication).orElse(null);
        var myGroupId = currentUserService.currentGroupId(authentication).orElse(null);
        var recipe = recipeService.getRecipe(token, myUserId, myGroupId);
        if (recipe.isEmpty()) {
            return "redirect:/dinner";
        }
        model.addAttribute("recipe", recipe.get());
        model.addAttribute("canModify", recipeService.canModify(recipe.get(), myUserId));
        return "recipe";
    }

    @GetMapping("/dinner")
    public String dinnerList(Authentication authentication, Model model) {
        var myUserId = currentUserService.currentUserId(authentication).orElse(null);
        var myGroupId = currentUserService.currentGroupId(authentication).orElse(null);
        var recipes = recipeService.getVisibleRecipes(myUserId, myGroupId);
        model.addAttribute("recipes", recipes);
        model.addAttribute("ingredientNames", recipeService.getAllIngredientNames(myUserId, myGroupId));
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

        var ownerId = currentUserService.currentUserId(authentication).orElseThrow();

        try {
            var saved = recipeService.saveForOwner(form, ownerId);
            return "redirect:/dinner/" + saved.getToken();
        } catch (RecipeLimitExceededException e) {
            model.addAttribute("limitError", e.getMessage());
            return "recipe-form";
        }
    }

    @GetMapping("/dinner/{token}/edit")
    public String editRecipeForm(@PathVariable String token, Authentication authentication, Model model) {
        var myUserId = currentUserService.currentUserId(authentication).orElse(null);
        var recipe = recipeService.getEditableRecipe(token, myUserId);
        if (recipe.isEmpty()) {
            return "redirect:/dinner";
        }

        if (!model.containsAttribute("recipeForm")) {
            model.addAttribute("recipeForm", toForm(recipe.get()));
        }
        model.addAttribute("token", token);
        return "recipe-form";
    }

    @PostMapping("/dinner/{token}/edit")
    public String update(@PathVariable String token, @Valid @ModelAttribute("recipeForm") RecipeForm form,
                          BindingResult bindingResult, Authentication authentication, Model model) {
        if (bindingResult.hasErrors()) {
            model.addAttribute("token", token);
            return "recipe-form";
        }

        var myUserId = currentUserService.currentUserId(authentication).orElse(null);
        if (!recipeService.updateForOwner(token, form, myUserId)) {
            return "redirect:/dinner";
        }
        return "redirect:/dinner/" + token;
    }

    @PostMapping("/dinner/{token}/delete")
    public String delete(@PathVariable String token, Authentication authentication) {
        var myUserId = currentUserService.currentUserId(authentication).orElse(null);
        recipeService.deleteForOwner(token, myUserId);
        return "redirect:/dinner";
    }

    private RecipeForm toForm(Recipe recipe) {
        var form = new RecipeForm();
        form.setTitle(recipe.getTitle());
        form.setIngredients(recipe.getIngredients().stream().map(Ingredient::name).collect(Collectors.toList()));
        form.setSteps(new ArrayList<>(recipe.getMethod()));
        return form;
    }
}
