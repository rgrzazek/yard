package dev.truckcode.yard.dinner;

import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.data.jpa.repository.Query;
import org.springframework.data.repository.query.Param;

import java.util.List;
import java.util.Optional;

public interface RecipeRepository extends JpaRepository<Recipe, Integer> {

    // myGroupId is always derived server-side from the logged-in user's
    // session, never trusted from the request — this is the whole access check,
    // for global and private recipes alike, one identifier scheme either way.
    @Query("SELECT r FROM Recipe r WHERE r.token = :token AND (r.isGlobal = true OR r.groupId = :myGroupId)")
    Optional<Recipe> findVisibleByToken(@Param("token") String token, @Param("myGroupId") Long myGroupId);

    // isGlobal check is defence in depth — the DB constraint doesn't guarantee global rows lack a group_id.
    @Query("SELECT r FROM Recipe r WHERE r.token = :token AND r.isGlobal = false AND r.groupId = :myGroupId")
    Optional<Recipe> findEditableByToken(@Param("token") String token, @Param("myGroupId") Long myGroupId);

    boolean existsByToken(String token);

    long countByGroupId(Long groupId);

    @Query("SELECT r FROM Recipe r WHERE r.isGlobal = true OR r.groupId = :myGroupId")
    List<Recipe> findAllVisibleTo(@Param("myGroupId") Long myGroupId);
}
