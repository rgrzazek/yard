package dev.truckcode.yard.auth;

import org.springframework.security.core.Authentication;
import org.springframework.stereotype.Controller;
import org.springframework.ui.Model;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.servlet.support.ServletUriComponentsBuilder;

@Controller
public class GroupController {

    private final GroupService groupService;
    private final CurrentUserService currentUserService;

    public GroupController(GroupService groupService, CurrentUserService currentUserService) {
        this.groupService = groupService;
        this.currentUserService = currentUserService;
    }

    @GetMapping("/group")
    public String group(Authentication authentication, Model model) {
        var user = currentUserService.currentUser(authentication).orElseThrow();
        if (user.getGroupId() == null) {
            return "group-setup";
        }

        var token = groupService.getOrCreateInviteToken(user.getGroupId());
        model.addAttribute("inviteUrl", inviteUrl(token));
        return "group";
    }

    @PostMapping("/group/start")
    public String start(Authentication authentication) {
        var user = currentUserService.currentUser(authentication).orElseThrow();
        groupService.startNewGroup(user);
        return "redirect:/group";
    }

    @PostMapping("/group/leave")
    public String leave(Authentication authentication) {
        var user = currentUserService.currentUser(authentication).orElseThrow();
        groupService.leaveGroup(user);
        return "redirect:/group";
    }

    @PostMapping("/group/invite/regenerate")
    public String regenerate(Authentication authentication) {
        var user = currentUserService.currentUser(authentication).orElseThrow();
        if (user.getGroupId() != null) {
            groupService.regenerateInviteToken(user.getGroupId());
        }
        return "redirect:/group";
    }

    // Read-only preview — actually joining needs a deliberate POST below.
    @GetMapping("/join/{token}")
    public String joinPreview(@PathVariable String token, Authentication authentication, Model model) {
        var user = currentUserService.currentUser(authentication).orElseThrow();
        if (user.getGroupId() != null) {
            model.addAttribute("result", "already-grouped");
        } else if (!groupService.canJoin(token)) {
            model.addAttribute("result", "invalid");
        } else {
            model.addAttribute("token", token);
        }
        return "join";
    }

    @PostMapping("/join/{token}")
    public String join(@PathVariable String token, Authentication authentication, Model model) {
        var user = currentUserService.currentUser(authentication).orElseThrow();
        if (user.getGroupId() != null) {
            model.addAttribute("result", "already-grouped");
        } else {
            model.addAttribute("result", groupService.joinGroup(user, token) ? "joined" : "invalid");
        }
        return "join";
    }

    private String inviteUrl(String token) {
        return ServletUriComponentsBuilder.fromCurrentContextPath()
                .path("/join/" + token)
                .toUriString();
    }
}
