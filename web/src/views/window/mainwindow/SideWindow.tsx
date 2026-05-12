import { defineComponent } from 'vue';
import { useBackendStore } from '@/stores';
import { WindowMode } from '@/types/windowinfo';

export default defineComponent({
  name: 'SideWindow',
  components: {

  },
  setup() {
    const backend = useBackendStore()
    // test
    const switchToMainWindow = () => {
       backend.requestWindow("switchMode", WindowMode.Main)
    }

    return {
        switchToMainWindow,
    };
  },
  render() {
    return (
      <div class="side-window">
        <div class="side-content">
          <div class="content-body">
            <h3>SideWindow</h3>
            <p>这是SideWindow组件的内容区域</p>
            <button onClick={this.switchToMainWindow}> 切换到主窗口 </button>
          </div>
        </div>
      </div>
    );
  }
});