package dev.truckcode.yard.dinner;

import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.data.jpa.repository.Query;
import org.springframework.data.repository.query.Param;

import java.util.List;
import java.util.Optional;

public interface RecipeRepository extends JpaRepository<Recipe, Integer> {

    // myUserId/myGroupId always come from the logged-in user's own session,
    // never the request. Visible = global, mine, or owned by anyone currently
    // in my household — sharing follows live group membership, not a snapshot.
    @Query("""
            SELECT r FROM Recipe r WHERE r.token = :token AND (r.isGlobal = true
                OR r.ownerId = :myUserId
                OR r.ownerId IN (SELECT u.id FROM User u WHERE u.groupId = :myGroupId))
            """)
    Optional<Recipe> findVisibleByToken(@Param("token") String token, @Param("myUserId") Long myUserId,
                                         @Param("myGroupId") Long myGroupId);

    // Editing/deleting stays with the actual owner — sharing a recipe with
    // your household doesn't hand over the right to change it.
    @Query("SELECT r FROM Recipe r WHERE r.token = :token AND r.isGlobal = false AND r.ownerId = :myUserId")
    Optional<Recipe> findEditableByToken(@Param("token") String token, @Param("myUserId") Long myUserId);

    boolean existsByToken(String token);

    long countByOwnerId(Long ownerId);

    @Query("""
            SELECT r FROM Recipe r WHERE r.isGlobal = true
                OR r.ownerId = :myUserId
                OR r.ownerId IN (SELECT u.id FROM User u WHERE u.groupId = :myGroupId)
            """)
    List<Recipe> findAllVisibleTo(@Param("myUserId") Long myUserId, @Param("myGroupId") Long myGroupId);
}
