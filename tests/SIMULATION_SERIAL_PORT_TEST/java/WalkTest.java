import com.example.walk.MixedRandomWalk;
import com.example.walk.OriginalWalk;
import org.junit.jupiter.api.Test;

public class WalkTest {

    @Test
    public void originalWalk() throws InterruptedException {
        OriginalWalk.walk();
    }

    @Test
    public void mixedRandomWalk() throws Exception {
        MixedRandomWalk.walk();
    }


}
