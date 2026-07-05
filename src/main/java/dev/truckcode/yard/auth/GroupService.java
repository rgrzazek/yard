package dev.truckcode.yard.auth;

import org.springframework.stereotype.Service;

import java.security.SecureRandom;
import java.util.Optional;

@Service
public class GroupService {

    private static final String TOKEN_CHARS = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    private static final int TOKEN_LENGTH = 10;

    private final UserRepository userRepository;
    private final GroupInviteRepository groupInviteRepository;
    private final SecureRandom random = new SecureRandom();

    public GroupService(UserRepository userRepository, GroupInviteRepository groupInviteRepository) {
        this.userRepository = userRepository;
        this.groupInviteRepository = groupInviteRepository;
    }

    public void startNewGroup(User user) {
        if (user.getGroupId() != null) return;
        user.setGroupId(userRepository.nextGroupId());
        userRepository.save(user);
    }

    // Recipes stay with their owner regardless of group membership, so
    // leaving is just this — nothing else needs to move.
    public void leaveGroup(User user) {
        user.setGroupId(null);
        userRepository.save(user);
    }

    public String getOrCreateInviteToken(Long groupId) {
        return groupInviteRepository.findById(groupId)
                .map(GroupInvite::getToken)
                .orElseGet(() -> regenerateInviteToken(groupId));
    }

    public String regenerateInviteToken(Long groupId) {
        var invite = groupInviteRepository.findById(groupId).orElseGet(GroupInvite::new);
        invite.setGroupId(groupId);
        invite.setToken(generateUniqueToken());
        return groupInviteRepository.save(invite).getToken();
    }

    // Read-only check for the confirmation page — never mutates.
    public boolean canJoin(String token) {
        return resolveJoinableGroup(token).isPresent();
    }

    // Only ever joins a groupless user — a user already in a household must
    // leave it deliberately elsewhere before this silently moves them, so it
    // just refuses instead.
    public boolean joinGroup(User user, String token) {
        if (user.getGroupId() != null) return false;

        var groupId = resolveJoinableGroup(token);
        if (groupId.isEmpty()) return false;

        user.setGroupId(groupId.get());
        userRepository.save(user);
        return true;
    }

    // Empty if the token doesn't exist, or its household is abandoned (every
    // member has since left) — refuses to let a stale link silently
    // resurrect one with just the new person in it.
    private Optional<Long> resolveJoinableGroup(String token) {
        return groupInviteRepository.findByToken(token)
                .map(GroupInvite::getGroupId)
                .filter(groupId -> userRepository.countByGroupId(groupId) > 0);
    }

    private String generateUniqueToken() {
        String candidate;
        do {
            var sb = new StringBuilder(TOKEN_LENGTH);
            for (int i = 0; i < TOKEN_LENGTH; i++) {
                sb.append(TOKEN_CHARS.charAt(random.nextInt(TOKEN_CHARS.length())));
            }
            candidate = sb.toString();
        } while (groupInviteRepository.existsByToken(candidate));
        return candidate;
    }
}
