-- Recipes now belong to the person who made them, not a household snapshot.
-- Household visibility is computed live via a join to app_user.group_id
-- instead of being stamped onto the recipe — so it follows someone if they
-- ever change households, and groupless people can own recipes too.
ALTER TABLE recipe ADD COLUMN owner_id BIGINT REFERENCES app_user (id);

-- Best-effort backfill: authorship was never recorded before this, so each
-- existing custom recipe is attributed to whichever member of its household
-- has the lowest user id. A guess, not a recovery of known data.
UPDATE recipe r
SET owner_id = (SELECT MIN(u.id) FROM app_user u WHERE u.group_id = r.group_id)
WHERE NOT r.is_global;

ALTER TABLE recipe DROP CONSTRAINT recipe_global_or_group_chk;
ALTER TABLE recipe ADD CONSTRAINT recipe_global_or_owner_chk CHECK (is_global OR owner_id IS NOT NULL);

ALTER TABLE recipe DROP COLUMN group_id;
