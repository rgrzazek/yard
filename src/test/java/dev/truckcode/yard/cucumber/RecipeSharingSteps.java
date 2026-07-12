package dev.truckcode.yard.cucumber;

import dev.truckcode.yard.auth.UserRepository;
import io.cucumber.java.en.But;
import io.cucumber.java.en.Given;
import io.cucumber.java.en.Then;
import io.cucumber.java.en.When;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.mock.web.MockHttpSession;
import org.springframework.test.web.servlet.MockMvc;
import org.springframework.test.web.servlet.MvcResult;

import java.util.HashMap;
import java.util.Map;

import static org.assertj.core.api.Assertions.assertThat;
import static org.springframework.security.test.web.servlet.request.SecurityMockMvcRequestPostProcessors.csrf;
import static org.springframework.test.web.servlet.request.MockMvcRequestBuilders.get;
import static org.springframework.test.web.servlet.request.MockMvcRequestBuilders.post;
import static org.springframework.test.web.servlet.result.MockMvcResultMatchers.redirectedUrl;
import static org.springframework.test.web.servlet.result.MockMvcResultMatchers.status;
import static org.springframework.test.web.servlet.result.MockMvcResultMatchers.view;

// cucumber-spring scopes this class to the scenario, so these maps never leak state across scenarios.
public class RecipeSharingSteps {

    private static final String PASSWORD = "s3cret-pw";

    @Autowired
    private MockMvc mockMvc;

    @Autowired
    private UserRepository userRepository;

    private final Map<String, MockHttpSession> sessions = new HashMap<>();
    private final Map<String, String> inviteTokens = new HashMap<>();
    private final Map<String, String> recipeTokens = new HashMap<>();

    @Given("{word} has signed up")
    public void hasSignedUp(String name) throws Exception {
        mockMvc.perform(post("/signup")
                        .param("username", name)
                        .param("password", PASSWORD)
                        .param("confirmPassword", PASSWORD)
                        .with(csrf()))
                .andExpect(status().isFound());

        MvcResult loginResult = mockMvc.perform(post("/login")
                        .param("username", name)
                        .param("password", PASSWORD)
                        .with(csrf()))
                .andExpect(status().isFound())
                .andReturn();

        sessions.put(name, (MockHttpSession) loginResult.getRequest().getSession());
    }

    @Given("{word} has signed up and started a new household")
    public void hasSignedUpAndStartedAHousehold(String name) throws Exception {
        hasSignedUp(name);
        mockMvc.perform(post("/group/start")
                        .session(sessions.get(name))
                        .with(csrf()))
                .andExpect(status().isFound());
    }

    @Given("{word} has generated an invite link for the household")
    public void hasGeneratedAnInviteLink(String name) throws Exception {
        MvcResult result = mockMvc.perform(get("/group").session(sessions.get(name)))
                .andExpect(status().isOk())
                .andReturn();

        String inviteUrl = (String) result.getModelAndView().getModel().get("inviteUrl");
        inviteTokens.put(name, inviteUrl.substring(inviteUrl.lastIndexOf('/') + 1));
    }

    @When("{word} joins {word}'s household using the invite link")
    public void joinsHouseholdUsingInviteLink(String joiner, String owner) throws Exception {
        mockMvc.perform(post("/join/{token}", inviteTokens.get(owner))
                        .session(sessions.get(joiner))
                        .with(csrf()))
                .andExpect(status().isOk());
    }

    @Then("{word} is a member of {word}'s household")
    public void isAMemberOfHousehold(String member, String owner) {
        var memberGroupId = userRepository.findByUsername(member).orElseThrow().getGroupId();
        var ownerGroupId = userRepository.findByUsername(owner).orElseThrow().getGroupId();
        assertThat(memberGroupId).isNotNull().isEqualTo(ownerGroupId);
    }

    @When("{word} creates the recipe {string}")
    public void createsTheRecipe(String name, String title) throws Exception {
        MvcResult result = mockMvc.perform(post("/dinner/save")
                        .param("title", title)
                        .session(sessions.get(name))
                        .with(csrf()))
                .andExpect(status().isFound())
                .andReturn();

        String redirect = result.getResponse().getRedirectedUrl();
        recipeTokens.put(title, redirect.substring(redirect.lastIndexOf('/') + 1));
    }

    @Then("{word} can see {string} in their recipe list")
    public void canSeeInRecipeList(String name, String title) throws Exception {
        assertThat(recipeListContains(name, title)).isTrue();
    }

    @Then("{word} can still see {string} in their recipe list")
    public void canStillSeeInRecipeList(String name, String title) throws Exception {
        canSeeInRecipeList(name, title);
    }

    @Then("{word} can no longer see {string}")
    public void canNoLongerSee(String name, String title) throws Exception {
        assertThat(recipeListContains(name, title)).isFalse();
    }

    @But("{word} cannot modify {string}")
    public void cannotModify(String name, String title) throws Exception {
        mockMvc.perform(get("/dinner/{token}/edit", recipeTokens.get(title))
                        .session(sessions.get(name)))
                .andExpect(status().isFound())
                .andExpect(redirectedUrl("/dinner"));
    }

    @Then("{word} can modify {string}")
    public void canModify(String name, String title) throws Exception {
        mockMvc.perform(get("/dinner/{token}/edit", recipeTokens.get(title))
                        .session(sessions.get(name)))
                .andExpect(status().isOk())
                .andExpect(view().name("recipe-form"));
    }

    @When("{word} leaves {word}'s household")
    public void leavesHousehold(String name, String owner) throws Exception {
        mockMvc.perform(post("/group/leave")
                        .session(sessions.get(name))
                        .with(csrf()))
                .andExpect(status().isFound());
    }

    private boolean recipeListContains(String name, String title) throws Exception {
        MvcResult result = mockMvc.perform(get("/dinner").session(sessions.get(name)))
                .andExpect(status().isOk())
                .andReturn();
        return result.getResponse().getContentAsString().contains(title);
    }
}
